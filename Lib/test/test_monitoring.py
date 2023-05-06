"""Test suite for the sys.monitoring."""

import collections
import functools
import operator
import sys
import types
import unittest


PAIR = (0,1)

def f1():
    pass

def f2():
    len([])
    sys.getsizeof(0)

def floop():
    for item in PAIR:
        pass

def gen():
    yield
    yield

def g1():
    for _ in gen():
        pass

TEST_TOOL = 2
TEST_TOOL2 = 3
TEST_TOOL3 = 4

class MonitoringBasicTest(unittest.TestCase):

    def test_has_objects(self):
        m = sys.monitoring
        m.events
        m.use_tool_id
        m.free_tool_id
        m.get_tool
        m.get_events
        m.set_events
        m.get_local_events
        m.set_local_events
        m.register_callback
        m.restart_events
        m.DISABLE
        m.MISSING
        m.events.NO_EVENTS

    def test_tool(self):
        sys.monitoring.use_tool_id(TEST_TOOL, "MonitoringTest.Tool")
        self.assertEqual(sys.monitoring.get_tool(TEST_TOOL), "MonitoringTest.Tool")
        sys.monitoring.set_events(TEST_TOOL, 15)
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL), 15)
        sys.monitoring.set_events(TEST_TOOL, 0)
        with self.assertRaises(ValueError):
            sys.monitoring.set_events(TEST_TOOL, sys.monitoring.events.C_RETURN)
        with self.assertRaises(ValueError):
            sys.monitoring.set_events(TEST_TOOL, sys.monitoring.events.C_RAISE)
        sys.monitoring.free_tool_id(TEST_TOOL)
        self.assertEqual(sys.monitoring.get_tool(TEST_TOOL), None)
        with self.assertRaises(ValueError):
            sys.monitoring.set_events(TEST_TOOL, sys.monitoring.events.CALL)


class MonitoringTestBase:

    def setUp(self):
        # Check that a previous test hasn't left monitoring on.
        for tool in range(6):
            self.assertEqual(sys.monitoring.get_events(tool), 0)
        self.assertIs(sys.monitoring.get_tool(TEST_TOOL), None)
        self.assertIs(sys.monitoring.get_tool(TEST_TOOL2), None)
        self.assertIs(sys.monitoring.get_tool(TEST_TOOL3), None)
        sys.monitoring.use_tool_id(TEST_TOOL, "test " + self.__class__.__name__)
        sys.monitoring.use_tool_id(TEST_TOOL2, "test2 " + self.__class__.__name__)
        sys.monitoring.use_tool_id(TEST_TOOL3, "test3 " + self.__class__.__name__)

    def tearDown(self):
        # Check that test hasn't left monitoring on.
        for tool in range(6):
            self.assertEqual(sys.monitoring.get_events(tool), 0)
        sys.monitoring.free_tool_id(TEST_TOOL)
        sys.monitoring.free_tool_id(TEST_TOOL2)
        sys.monitoring.free_tool_id(TEST_TOOL3)


class MonitoringCountTest(MonitoringTestBase, unittest.TestCase):

    def check_event_count(self, func, event, expected):

        class Counter:
            def __init__(self):
                self.count = 0
            def __call__(self, *args):
                self.count += 1

        counter = Counter()
        sys.monitoring.register_callback(TEST_TOOL, event, counter)
        if event == E.C_RETURN or event == E.C_RAISE:
            sys.monitoring.set_events(TEST_TOOL, E.CALL)
        else:
            sys.monitoring.set_events(TEST_TOOL, event)
        self.assertEqual(counter.count, 0)
        counter.count = 0
        func()
        self.assertEqual(counter.count, expected)
        prev = sys.monitoring.register_callback(TEST_TOOL, event, None)
        counter.count = 0
        func()
        self.assertEqual(counter.count, 0)
        self.assertEqual(prev, counter)
        sys.monitoring.set_events(TEST_TOOL, 0)

    def test_start_count(self):
        self.check_event_count(f1, E.PY_START, 1)

    def test_resume_count(self):
        self.check_event_count(g1, E.PY_RESUME, 2)

    def test_return_count(self):
        self.check_event_count(f1, E.PY_RETURN, 1)

    def test_call_count(self):
        self.check_event_count(f2, E.CALL, 3)

    def test_c_return_count(self):
        self.check_event_count(f2, E.C_RETURN, 2)


E = sys.monitoring.events

SIMPLE_EVENTS = [
    (E.PY_START, "start"),
    (E.PY_RESUME, "resume"),
    (E.PY_RETURN, "return"),
    (E.PY_YIELD, "yield"),
    (E.JUMP, "jump"),
    (E.BRANCH, "branch"),
    (E.RAISE, "raise"),
    (E.PY_UNWIND, "unwind"),
    (E.EXCEPTION_HANDLED, "exception_handled"),
    (E.C_RAISE, "c_raise"),
    (E.C_RETURN, "c_return"),
]

SIMPLE_EVENT_SET = functools.reduce(operator.or_, [ev for (ev, _) in SIMPLE_EVENTS], 0) | E.CALL


def just_pass():
    pass

just_pass.events = [
    "py_call",
    "start",
    "return",
]

def just_raise():
    raise Exception

just_raise.events = [
    'py_call',
    "start",
    "raise",
    "unwind",
]

def just_call():
    len([])

just_call.events = [
    'py_call',
    "start",
    "c_call",
    "c_return",
    "return",
]

def caught():
    try:
        1/0
    except Exception:
        pass

caught.events = [
    'py_call',
    "start",
    "raise",
    "exception_handled",
    "branch",
    "return",
]

def nested_call():
    just_pass()

nested_call.events = [
    "py_call",
    "start",
    "py_call",
    "start",
    "return",
    "return",
]

PY_CALLABLES = (types.FunctionType, types.MethodType)

class MonitoringEventsBase(MonitoringTestBase):

    def gather_events(self, func):
        events = []
        for event, event_name in SIMPLE_EVENTS:
            def record(*args, event_name=event_name):
                events.append(event_name)
            sys.monitoring.register_callback(TEST_TOOL, event, record)
        def record_call(code, offset, obj, arg):
            if isinstance(obj, PY_CALLABLES):
                events.append("py_call")
            else:
                events.append("c_call")
        sys.monitoring.register_callback(TEST_TOOL, E.CALL, record_call)
        sys.monitoring.set_events(TEST_TOOL, SIMPLE_EVENT_SET)
        events = []
        try:
            func()
        except:
            pass
        sys.monitoring.set_events(TEST_TOOL, 0)
        #Remove the final event, the call to `sys.monitoring.set_events`
        events = events[:-1]
        return events

    def check_events(self, func, expected=None):
        events = self.gather_events(func)
        if expected is None:
            expected = func.events
        self.assertEqual(events, expected)


class MonitoringEventsTest(MonitoringEventsBase, unittest.TestCase):

    def test_just_pass(self):
        self.check_events(just_pass)

    def test_just_raise(self):
        try:
            self.check_events(just_raise)
        except Exception:
            pass
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL), 0)

    def test_just_call(self):
        self.check_events(just_call)

    def test_caught(self):
        self.check_events(caught)

    def test_nested_call(self):
        self.check_events(nested_call)

UP_EVENTS = (E.C_RETURN, E.C_RAISE, E.PY_RETURN, E.PY_UNWIND, E.PY_YIELD)
DOWN_EVENTS = (E.PY_START, E.PY_RESUME)

from test.profilee import testfunc

class SimulateProfileTest(MonitoringEventsBase, unittest.TestCase):

    def test_balanced(self):
        events = self.gather_events(testfunc)
        c = collections.Counter(events)
        self.assertEqual(c["c_call"], c["c_return"])
        self.assertEqual(c["start"], c["return"] + c["unwind"])
        self.assertEqual(c["raise"], c["exception_handled"] + c["unwind"])

    def test_frame_stack(self):
        self.maxDiff = None
        stack = []
        errors = []
        seen = set()
        def up(*args):
            frame = sys._getframe(1)
            if not stack:
                errors.append("empty")
            else:
                expected = stack.pop()
                if frame != expected:
                    errors.append(f" Popping {frame} expected {expected}")
        def down(*args):
            frame = sys._getframe(1)
            stack.append(frame)
            seen.add(frame.f_code)
        def call(code, offset, callable, arg):
            if not isinstance(callable, PY_CALLABLES):
                stack.append(sys._getframe(1))
        for event in UP_EVENTS:
            sys.monitoring.register_callback(TEST_TOOL, event, up)
        for event in DOWN_EVENTS:
            sys.monitoring.register_callback(TEST_TOOL, event, down)
        sys.monitoring.register_callback(TEST_TOOL, E.CALL, call)
        sys.monitoring.set_events(TEST_TOOL, SIMPLE_EVENT_SET)
        testfunc()
        sys.monitoring.set_events(TEST_TOOL, 0)
        self.assertEqual(errors, [])
        self.assertEqual(stack, [sys._getframe()])
        self.assertEqual(len(seen), 9)


class CounterWithDisable:

    def __init__(self):
        self.disable = False
        self.count = 0

    def __call__(self, *args):
        self.count += 1
        if self.disable:
            return sys.monitoring.DISABLE


class RecorderWithDisable:

    def __init__(self, events):
        self.disable = False
        self.events = events

    def __call__(self, code, event):
        self.events.append(event)
        if self.disable:
            return sys.monitoring.DISABLE


class MontoringDisableAndRestartTest(MonitoringTestBase, unittest.TestCase):

    def test_disable(self):
        try:
            counter = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            self.assertEqual(counter.count, 0)
            counter.count = 0
            f1()
            self.assertEqual(counter.count, 1)
            counter.disable = True
            counter.count = 0
            f1()
            self.assertEqual(counter.count, 1)
            counter.count = 0
            f1()
            self.assertEqual(counter.count, 0)
            sys.monitoring.set_events(TEST_TOOL, 0)
        finally:
            sys.monitoring.restart_events()

    def test_restart(self):
        try:
            counter = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            counter.disable = True
            f1()
            counter.count = 0
            f1()
            self.assertEqual(counter.count, 0)
            sys.monitoring.restart_events()
            counter.count = 0
            f1()
            self.assertEqual(counter.count, 1)
            sys.monitoring.set_events(TEST_TOOL, 0)
        finally:
            sys.monitoring.restart_events()


class MultipleMonitorsTest(MonitoringTestBase, unittest.TestCase):

    def test_two_same(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, counter2)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_START)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': (1 << TEST_TOOL) | (1 << TEST_TOOL2)})
            counter1.count = 0
            counter2.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (1, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, None)
            self.assertEqual(sys.monitoring._all_events(), {})

    def test_three_same(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            counter3 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, counter2)
            sys.monitoring.register_callback(TEST_TOOL3, E.PY_START, counter3)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL3, E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL3), E.PY_START)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': (1 << TEST_TOOL) | (1 << TEST_TOOL2) | (1 << TEST_TOOL3)})
            counter1.count = 0
            counter2.count = 0
            counter3.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            count3 = counter3.count
            self.assertEqual((count1, count2, count3), (1, 1, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.set_events(TEST_TOOL3, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL3, E.PY_START, None)
            self.assertEqual(sys.monitoring._all_events(), {})

    def test_two_different(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_RETURN, counter2)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_RETURN)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_RETURN)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': 1 << TEST_TOOL, 'PY_RETURN': 1 << TEST_TOOL2})
            counter1.count = 0
            counter2.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (1, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_RETURN, None)
            self.assertEqual(sys.monitoring._all_events(), {})

    def test_two_with_disable(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, counter2)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_START)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': (1 << TEST_TOOL) | (1 << TEST_TOOL2)})
            counter1.count = 0
            counter2.count = 0
            counter1.disable = True
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (1, 1))
            counter1.count = 0
            counter2.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (0, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()

class LineMonitoringTest(MonitoringTestBase, unittest.TestCase):

    def test_lines_single(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            events = []
            recorder = RecorderWithDisable(events)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, recorder)
            sys.monitoring.set_events(TEST_TOOL, E.LINE)
            f1()
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            start = LineMonitoringTest.test_lines_single.__code__.co_firstlineno
            self.assertEqual(events, [start+7, 14, start+8])
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()

    def test_lines_loop(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            events = []
            recorder = RecorderWithDisable(events)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, recorder)
            sys.monitoring.set_events(TEST_TOOL, E.LINE)
            floop()
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            start = LineMonitoringTest.test_lines_loop.__code__.co_firstlineno
            self.assertEqual(events, [start+7, 21, 22, 22, 21, start+8])
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()

    def test_lines_two(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            events = []
            recorder = RecorderWithDisable(events)
            events2 = []
            recorder2 = RecorderWithDisable(events2)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, recorder)
            sys.monitoring.register_callback(TEST_TOOL2, E.LINE, recorder2)
            sys.monitoring.set_events(TEST_TOOL, E.LINE); sys.monitoring.set_events(TEST_TOOL2, E.LINE)
            f1()
            sys.monitoring.set_events(TEST_TOOL, 0); sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.LINE, None)
            start = LineMonitoringTest.test_lines_two.__code__.co_firstlineno
            expected = [start+10, 14, start+11]
            self.assertEqual(events, expected)
            self.assertEqual(events2, expected)
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.LINE, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()

    def check_lines(self, func, expected, tool=TEST_TOOL):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            events = []
            recorder = RecorderWithDisable(events)
            sys.monitoring.register_callback(tool, E.LINE, recorder)
            sys.monitoring.set_events(tool, E.LINE)
            func()
            sys.monitoring.set_events(tool, 0)
            sys.monitoring.register_callback(tool, E.LINE, None)
            lines = [ line - func.__code__.co_firstlineno for line in events[1:-1] ]
            self.assertEqual(lines, expected)
        finally:
            sys.monitoring.set_events(tool, 0)


    def test_linear(self):

        def func():
            line = 1
            line = 2
            line = 3
            line = 4
            line = 5

        self.check_lines(func, [1,2,3,4,5])

    def test_branch(self):
        def func():
            if "true".startswith("t"):
                line = 2
                line = 3
            else:
                line = 5
            line = 6

        self.check_lines(func, [1,2,3,6])

    def test_try_except(self):

        def func1():
            try:
                line = 2
                line = 3
            except:
                line = 5
            line = 6

        self.check_lines(func1, [1,2,3,6])

        def func2():
            try:
                line = 2
                raise 3
            except:
                line = 5
            line = 6

        self.check_lines(func2, [1,2,3,4,5,6])


class ExceptionRecorder:

    event_type = E.RAISE

    def __init__(self, events):
        self.events = events

    def __call__(self, code, offset, exc):
        self.events.append(("raise", type(exc)))

class CheckEvents(MonitoringTestBase, unittest.TestCase):

    def check_events(self, func, expected, tool=TEST_TOOL, recorders=(ExceptionRecorder,)):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            event_list = []
            all_events = 0
            for recorder in recorders:
                ev = recorder.event_type
                sys.monitoring.register_callback(tool, ev, recorder(event_list))
                all_events |= ev
            sys.monitoring.set_events(tool, all_events)
            func()
            sys.monitoring.set_events(tool, 0)
            for recorder in recorders:
                sys.monitoring.register_callback(tool, recorder.event_type, None)
            self.assertEqual(event_list, expected)
        finally:
            sys.monitoring.set_events(tool, 0)
            for recorder in recorders:
                sys.monitoring.register_callback(tool, recorder.event_type, None)

class StopiterationRecorder(ExceptionRecorder):

    event_type = E.STOP_ITERATION

class ExceptionMontoringTest(CheckEvents):

    recorder = ExceptionRecorder

    def test_simple_try_except(self):

        def func1():
            try:
                line = 2
                raise KeyError
            except:
                line = 5
            line = 6

        self.check_events(func1, [("raise", KeyError)])

        def gen():
            yield 1
            return 2

        def implicit_stop_iteration():
            for _ in gen():
                pass

        self.check_events(implicit_stop_iteration, [("raise", StopIteration)], recorders=(StopiterationRecorder,))

class LineRecorder:

    event_type = E.LINE


    def __init__(self, events):
        self.events = events

    def __call__(self, code, line):
        self.events.append(("line", code.co_name, line - code.co_firstlineno))

class CallRecorder:

    event_type = E.CALL

    def __init__(self, events):
        self.events = events

    def __call__(self, code, offset, func, arg):
        self.events.append(("call", func.__name__, arg))

class CEventRecorder:

    def __init__(self, events):
        self.events = events

    def __call__(self, code, offset, func, arg):
        self.events.append((self.event_name, func.__name__, arg))

class CReturnRecorder(CEventRecorder):

    event_type = E.C_RETURN
    event_name = "C return"

class CRaiseRecorder(CEventRecorder):

    event_type = E.C_RAISE
    event_name = "C raise"

MANY_RECORDERS = ExceptionRecorder, CallRecorder, LineRecorder, CReturnRecorder, CRaiseRecorder

class TestManyEvents(CheckEvents):

    def test_simple(self):

        def func1():
            line1 = 1
            line2 = 2
            line3 = 3

        self.check_events(func1, recorders = MANY_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('call', 'func1', sys.monitoring.MISSING),
            ('line', 'func1', 1),
            ('line', 'func1', 2),
            ('line', 'func1', 3),
            ('line', 'check_events', 11),
            ('call', 'set_events', 2)])

    def test_c_call(self):

        def func2():
            line1 = 1
            [].append(2)
            line3 = 3

        self.check_events(func2, recorders = MANY_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('call', 'func2', sys.monitoring.MISSING),
            ('line', 'func2', 1),
            ('line', 'func2', 2),
            ('call', 'append', [2]),
            ('C return', 'append', [2]),
            ('line', 'func2', 3),
            ('line', 'check_events', 11),
            ('call', 'set_events', 2)])

    def test_try_except(self):

        def func3():
            try:
                line = 2
                raise KeyError
            except:
                line = 5
            line = 6

        self.check_events(func3, recorders = MANY_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('call', 'func3', sys.monitoring.MISSING),
            ('line', 'func3', 1),
            ('line', 'func3', 2),
            ('line', 'func3', 3),
            ('raise', KeyError),
            ('line', 'func3', 4),
            ('line', 'func3', 5),
            ('line', 'func3', 6),
            ('line', 'check_events', 11),
            ('call', 'set_events', 2)])

class InstructionRecorder:

    event_type = E.INSTRUCTION

    def __init__(self, events):
        self.events = events

    def __call__(self, code, offset):
        # Filter out instructions in check_events to lower noise
        if code.co_name != "check_events":
            self.events.append(("instruction", code.co_name, offset))


LINE_AND_INSTRUCTION_RECORDERS = InstructionRecorder, LineRecorder

class TestLineAndInstructionEvents(CheckEvents):
    maxDiff = None

    def test_simple(self):

        def func1():
            line1 = 1
            line2 = 2
            line3 = 3

        self.check_events(func1, recorders = LINE_AND_INSTRUCTION_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('line', 'func1', 1),
            ('instruction', 'func1', 2),
            ('instruction', 'func1', 4),
            ('line', 'func1', 2),
            ('instruction', 'func1', 6),
            ('instruction', 'func1', 8),
            ('line', 'func1', 3),
            ('instruction', 'func1', 10),
            ('instruction', 'func1', 12),
            ('instruction', 'func1', 14),
            ('line', 'check_events', 11)])

    def test_c_call(self):

        def func2():
            line1 = 1
            [].append(2)
            line3 = 3

        self.check_events(func2, recorders = LINE_AND_INSTRUCTION_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('line', 'func2', 1),
            ('instruction', 'func2', 2),
            ('instruction', 'func2', 4),
            ('line', 'func2', 2),
            ('instruction', 'func2', 6),
            ('instruction', 'func2', 8),
            ('instruction', 'func2', 28),
            ('instruction', 'func2', 30),
            ('instruction', 'func2', 38),
            ('line', 'func2', 3),
            ('instruction', 'func2', 40),
            ('instruction', 'func2', 42),
            ('instruction', 'func2', 44),
            ('line', 'check_events', 11)])

    def test_try_except(self):

        def func3():
            try:
                line = 2
                raise KeyError
            except:
                line = 5
            line = 6

        self.check_events(func3, recorders = LINE_AND_INSTRUCTION_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('line', 'func3', 1),
            ('instruction', 'func3', 2),
            ('line', 'func3', 2),
            ('instruction', 'func3', 4),
            ('instruction', 'func3', 6),
            ('line', 'func3', 3),
            ('instruction', 'func3', 8),
            ('instruction', 'func3', 18),
            ('instruction', 'func3', 20),
            ('line', 'func3', 4),
            ('instruction', 'func3', 22),
            ('line', 'func3', 5),
            ('instruction', 'func3', 24),
            ('instruction', 'func3', 26),
            ('instruction', 'func3', 28),
            ('line', 'func3', 6),
            ('instruction', 'func3', 30),
            ('instruction', 'func3', 32),
            ('instruction', 'func3', 34),
            ('line', 'check_events', 11)])

    def test_with_restart(self):
        def func1():
            line1 = 1
            line2 = 2
            line3 = 3

        self.check_events(func1, recorders = LINE_AND_INSTRUCTION_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('line', 'func1', 1),
            ('instruction', 'func1', 2),
            ('instruction', 'func1', 4),
            ('line', 'func1', 2),
            ('instruction', 'func1', 6),
            ('instruction', 'func1', 8),
            ('line', 'func1', 3),
            ('instruction', 'func1', 10),
            ('instruction', 'func1', 12),
            ('instruction', 'func1', 14),
            ('line', 'check_events', 11)])

        sys.monitoring.restart_events()

        self.check_events(func1, recorders = LINE_AND_INSTRUCTION_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('line', 'func1', 1),
            ('instruction', 'func1', 2),
            ('instruction', 'func1', 4),
            ('line', 'func1', 2),
            ('instruction', 'func1', 6),
            ('instruction', 'func1', 8),
            ('line', 'func1', 3),
            ('instruction', 'func1', 10),
            ('instruction', 'func1', 12),
            ('instruction', 'func1', 14),
            ('line', 'check_events', 11)])

class TestInstallIncrementallly(MonitoringTestBase, unittest.TestCase):

    def check_events(self, func, must_include, tool=TEST_TOOL, recorders=(ExceptionRecorder,)):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            event_list = []
            all_events = 0
            for recorder in recorders:
                all_events |= recorder.event_type
                sys.monitoring.set_events(tool, all_events)
            for recorder in recorders:
                sys.monitoring.register_callback(tool, recorder.event_type, recorder(event_list))
            func()
            sys.monitoring.set_events(tool, 0)
            for recorder in recorders:
                sys.monitoring.register_callback(tool, recorder.event_type, None)
            for line in must_include:
                self.assertIn(line, event_list)
        finally:
            sys.monitoring.set_events(tool, 0)
            for recorder in recorders:
                sys.monitoring.register_callback(tool, recorder.event_type, None)

    @staticmethod
    def func1():
        line1 = 1

    MUST_INCLUDE_LI = [
            ('instruction', 'func1', 2),
            ('line', 'func1', 1),
            ('instruction', 'func1', 4),
            ('instruction', 'func1', 6)]

    def test_line_then_instruction(self):
        recorders = [ LineRecorder, InstructionRecorder ]
        self.check_events(self.func1,
                          recorders = recorders, must_include = self.EXPECTED_LI)

    def test_instruction_then_line(self):
        recorders = [ InstructionRecorder, LineRecorderLowNoise ]
        self.check_events(self.func1,
                          recorders = recorders, must_include = self.EXPECTED_LI)

    @staticmethod
    def func2():
        len(())

    MUST_INCLUDE_CI = [
            ('instruction', 'func2', 2),
            ('call', 'func2', sys.monitoring.MISSING),
            ('call', 'len', ()),
            ('instruction', 'func2', 12),
            ('instruction', 'func2', 14)]



    def test_line_then_instruction(self):
        recorders = [ CallRecorder, InstructionRecorder ]
        self.check_events(self.func2,
                          recorders = recorders, must_include = self.MUST_INCLUDE_CI)

    def test_instruction_then_line(self):
        recorders = [ InstructionRecorder, CallRecorder ]
        self.check_events(self.func2,
                          recorders = recorders, must_include = self.MUST_INCLUDE_CI)

class TestLocalEvents(MonitoringTestBase, unittest.TestCase):

    def check_events(self, func, expected, tool=TEST_TOOL, recorders=(ExceptionRecorder,)):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            event_list = []
            all_events = 0
            for recorder in recorders:
                ev = recorder.event_type
                sys.monitoring.register_callback(tool, ev, recorder(event_list))
                all_events |= ev
            sys.monitoring.set_local_events(tool, func.__code__, all_events)
            func()
            sys.monitoring.set_local_events(tool, func.__code__, 0)
            for recorder in recorders:
                sys.monitoring.register_callback(tool, recorder.event_type, None)
            self.assertEqual(event_list, expected)
        finally:
            sys.monitoring.set_local_events(tool, func.__code__, 0)
            for recorder in recorders:
                sys.monitoring.register_callback(tool, recorder.event_type, None)


    def test_simple(self):

        def func1():
            line1 = 1
            line2 = 2
            line3 = 3

        self.check_events(func1, recorders = MANY_RECORDERS, expected = [
            ('line', 'func1', 1),
            ('line', 'func1', 2),
            ('line', 'func1', 3)])

    def test_c_call(self):

        def func2():
            line1 = 1
            [].append(2)
            line3 = 3

        self.check_events(func2, recorders = MANY_RECORDERS, expected = [
            ('line', 'func2', 1),
            ('line', 'func2', 2),
            ('call', 'append', [2]),
            ('C return', 'append', [2]),
            ('line', 'func2', 3)])

    def test_try_except(self):

        def func3():
            try:
                line = 2
                raise KeyError
            except:
                line = 5
            line = 6

        self.check_events(func3, recorders = MANY_RECORDERS, expected = [
            ('line', 'func3', 1),
            ('line', 'func3', 2),
            ('line', 'func3', 3),
            ('raise', KeyError),
            ('line', 'func3', 4),
            ('line', 'func3', 5),
            ('line', 'func3', 6)])


def line_from_offset(code, offset):
    for start, end, line in code.co_lines():
        if start <= offset < end:
            return line - code.co_firstlineno
    return -1

class JumpRecorder:

    event_type = E.JUMP
    name = "jump"

    def __init__(self, events):
        self.events = events

    def __call__(self, code, from_, to):
        from_line = line_from_offset(code, from_)
        to_line = line_from_offset(code, to)
        self.events.append((self.name, code.co_name, from_line, to_line))


class BranchRecorder(JumpRecorder):

    event_type = E.BRANCH
    name = "branch"


JUMP_AND_BRANCH_RECORDERS = JumpRecorder, BranchRecorder
JUMP_BRANCH_AND_LINE_RECORDERS = JumpRecorder, BranchRecorder, LineRecorder

class TestBranchAndJumpEvents(CheckEvents):
    maxDiff = None

    def test_loop(self):

        def func():
            x = 1
            for a in range(2):
                if a:
                    x = 4
                else:
                    x = 6

        self.check_events(func, recorders = JUMP_AND_BRANCH_RECORDERS, expected = [
            ('branch', 'func', 2, 2),
            ('branch', 'func', 3, 6),
            ('jump', 'func', 6, 2),
            ('branch', 'func', 2, 2),
            ('branch', 'func', 3, 4),
            ('jump', 'func', 4, 2),
            ('branch', 'func', 2, 2)])


        self.check_events(func, recorders = JUMP_BRANCH_AND_LINE_RECORDERS, expected = [
            ('line', 'check_events', 10),
            ('line', 'func', 1),
            ('line', 'func', 2),
            ('branch', 'func', 2, 2),
            ('line', 'func', 3),
            ('branch', 'func', 3, 6),
            ('line', 'func', 6),
            ('jump', 'func', 6, 2),
            ('branch', 'func', 2, 2),
            ('line', 'func', 3),
            ('branch', 'func', 3, 4),
            ('line', 'func', 4),
            ('jump', 'func', 4, 2),
            ('branch', 'func', 2, 2),
            ('line', 'func', 2),
            ('line', 'check_events', 11)])


class TestSetGetEvents(MonitoringTestBase, unittest.TestCase):

    def test_global(self):
        sys.monitoring.set_events(TEST_TOOL, E.PY_START)
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
        sys.monitoring.set_events(TEST_TOOL2, E.PY_START)
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_START)
        sys.monitoring.set_events(TEST_TOOL, 0)
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL), 0)
        sys.monitoring.set_events(TEST_TOOL2,0)
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), 0)

    def test_local(self):
        code = f1.__code__
        sys.monitoring.set_local_events(TEST_TOOL, code, E.PY_START)
        self.assertEqual(sys.monitoring.get_local_events(TEST_TOOL, code), E.PY_START)
        sys.monitoring.set_local_events(TEST_TOOL2, code, E.PY_START)
        self.assertEqual(sys.monitoring.get_local_events(TEST_TOOL2, code), E.PY_START)
        sys.monitoring.set_local_events(TEST_TOOL, code, 0)
        self.assertEqual(sys.monitoring.get_local_events(TEST_TOOL, code), 0)
        sys.monitoring.set_local_events(TEST_TOOL2, code, 0)
        self.assertEqual(sys.monitoring.get_local_events(TEST_TOOL2, code), 0)

class TestUninitialized(unittest.TestCase, MonitoringTestBase):

    @staticmethod
    def f():
        pass

    def test_get_local_events_uninitialized(self):
        self.assertEqual(sys.monitoring.get_local_events(TEST_TOOL, self.f.__code__), 0)
