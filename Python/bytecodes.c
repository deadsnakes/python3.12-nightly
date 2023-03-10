// This file contains instruction definitions.
// It is read by Tools/cases_generator/generate_cases.py
// to generate Python/generated_cases.c.h.
// Note that there is some dummy C code at the top and bottom of the file
// to fool text editors like VS Code into believing this is valid C code.
// The actual instruction definitions start at // BEGIN BYTECODES //.
// See Tools/cases_generator/README.md for more information.

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_call.h"          // _PyObject_FastCallDictTstate()
#include "pycore_ceval.h"         // _PyEval_SignalAsyncExc()
#include "pycore_code.h"
#include "pycore_function.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_opcode.h"        // EXTRA_CASES
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pymem.h"         // _PyMem_IsPtrFreed()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_range.h"         // _PyRangeIterObject
#include "pycore_sliceobject.h"   // _PyBuildSlice_ConsumeRefs
#include "pycore_sysmodule.h"     // _PySys_Audit()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()
#include "pycore_emscripten_signal.h"  // _Py_CHECK_EMSCRIPTEN_SIGNALS

#include "pycore_dict.h"
#include "dictobject.h"
#include "pycore_frame.h"
#include "opcode.h"
#include "pydtrace.h"
#include "setobject.h"
#include "structmember.h"         // struct PyMemberDef, T_OFFSET_EX

#define USE_COMPUTED_GOTOS 0
#include "ceval_macros.h"

/* Flow control macros */
#define DEOPT_IF(cond, instname) ((void)0)
#define ERROR_IF(cond, labelname) ((void)0)
#define GO_TO_INSTRUCTION(instname) ((void)0)
#define PREDICT(opname) ((void)0)

#define inst(name, ...) case name:
#define op(name, ...) /* NAME is ignored */
#define macro(name) static int MACRO_##name
#define super(name) static int SUPER_##name
#define family(name, ...) static int family_##name

// Dummy variables for stack effects.
static PyObject *value, *value1, *value2, *left, *right, *res, *sum, *prod, *sub;
static PyObject *container, *start, *stop, *v, *lhs, *rhs, *res2;
static PyObject *list, *tuple, *dict, *owner, *set, *str, *tup, *map, *keys;
static PyObject *exit_func, *lasti, *val, *retval, *obj, *iter;
static PyObject *aiter, *awaitable, *iterable, *w, *exc_value, *bc;
static PyObject *orig, *excs, *update, *b, *fromlist, *level, *from;
static PyObject **pieces, **values;
static size_t jump;
// Dummy variables for cache effects
static uint16_t invert, counter, index, hint;
static uint32_t type_version;

static PyObject *
dummy_func(
    PyThreadState *tstate,
    _PyInterpreterFrame *frame,
    unsigned char opcode,
    unsigned int oparg,
    _Py_atomic_int * const eval_breaker,
    _PyCFrame cframe,
    PyObject *names,
    PyObject *consts,
    _Py_CODEUNIT *next_instr,
    PyObject **stack_pointer,
    PyObject *kwnames,
    int throwflag,
    binaryfunc binary_ops[]
)
{
    _PyInterpreterFrame  entry_frame;

    switch (opcode) {

// BEGIN BYTECODES //
        inst(NOP, (--)) {
        }

        inst(RESUME, (--)) {
            assert(tstate->cframe == &cframe);
            assert(frame == cframe.current_frame);
            if (_Py_atomic_load_relaxed_int32(eval_breaker) && oparg < 2) {
                goto handle_eval_breaker;
            }
        }

        inst(LOAD_CLOSURE, (-- value)) {
            /* We keep LOAD_CLOSURE so that the bytecode stays more readable. */
            value = GETLOCAL(oparg);
            ERROR_IF(value == NULL, unbound_local_error);
            Py_INCREF(value);
        }

        inst(LOAD_FAST_CHECK, (-- value)) {
            value = GETLOCAL(oparg);
            ERROR_IF(value == NULL, unbound_local_error);
            Py_INCREF(value);
        }

        inst(LOAD_FAST, (-- value)) {
            value = GETLOCAL(oparg);
            assert(value != NULL);
            Py_INCREF(value);
        }

        inst(LOAD_CONST, (-- value)) {
            value = GETITEM(consts, oparg);
            Py_INCREF(value);
        }

        inst(STORE_FAST, (value --)) {
            SETLOCAL(oparg, value);
        }

        super(LOAD_FAST__LOAD_FAST) = LOAD_FAST + LOAD_FAST;
        super(LOAD_FAST__LOAD_CONST) = LOAD_FAST + LOAD_CONST;
        super(STORE_FAST__LOAD_FAST)  = STORE_FAST + LOAD_FAST;
        super(STORE_FAST__STORE_FAST) = STORE_FAST + STORE_FAST;
        super(LOAD_CONST__LOAD_FAST) = LOAD_CONST + LOAD_FAST;

        inst(POP_TOP, (value --)) {
            DECREF_INPUTS();
        }

        inst(PUSH_NULL, (-- res)) {
            res = NULL;
        }

        macro(END_FOR) = POP_TOP + POP_TOP;

        inst(UNARY_NEGATIVE, (value -- res)) {
            res = PyNumber_Negative(value);
            DECREF_INPUTS();
            ERROR_IF(res == NULL, error);
        }

        inst(UNARY_NOT, (value -- res)) {
            int err = PyObject_IsTrue(value);
            DECREF_INPUTS();
            ERROR_IF(err < 0, error);
            if (err == 0) {
                res = Py_True;
            }
            else {
                res = Py_False;
            }
            Py_INCREF(res);
        }

        inst(UNARY_INVERT, (value -- res)) {
            res = PyNumber_Invert(value);
            DECREF_INPUTS();
            ERROR_IF(res == NULL, error);
        }

        family(binary_op, INLINE_CACHE_ENTRIES_BINARY_OP) = {
            BINARY_OP,
            BINARY_OP_ADD_FLOAT,
            BINARY_OP_ADD_INT,
            BINARY_OP_ADD_UNICODE,
            // BINARY_OP_INPLACE_ADD_UNICODE,  // This is an odd duck.
            BINARY_OP_MULTIPLY_FLOAT,
            BINARY_OP_MULTIPLY_INT,
            BINARY_OP_SUBTRACT_FLOAT,
            BINARY_OP_SUBTRACT_INT,
        };


        inst(BINARY_OP_MULTIPLY_INT, (unused/1, left, right -- prod)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyLong_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            prod = _PyLong_Multiply((PyLongObject *)left, (PyLongObject *)right);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            ERROR_IF(prod == NULL, error);
        }

        inst(BINARY_OP_MULTIPLY_FLOAT, (unused/1, left, right -- prod)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyFloat_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dprod = ((PyFloatObject *)left)->ob_fval *
                ((PyFloatObject *)right)->ob_fval;
            prod = PyFloat_FromDouble(dprod);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            ERROR_IF(prod == NULL, error);
        }

        inst(BINARY_OP_SUBTRACT_INT, (unused/1, left, right -- sub)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyLong_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            sub = _PyLong_Subtract((PyLongObject *)left, (PyLongObject *)right);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            ERROR_IF(sub == NULL, error);
        }

        inst(BINARY_OP_SUBTRACT_FLOAT, (unused/1, left, right -- sub)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(!PyFloat_CheckExact(right), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dsub = ((PyFloatObject *)left)->ob_fval - ((PyFloatObject *)right)->ob_fval;
            sub = PyFloat_FromDouble(dsub);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            ERROR_IF(sub == NULL, error);
        }

        inst(BINARY_OP_ADD_UNICODE, (unused/1, left, right -- res)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyUnicode_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            res = PyUnicode_Concat(left, right);
            _Py_DECREF_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            ERROR_IF(res == NULL, error);
        }

        // This is a subtle one. It's a super-instruction for
        // BINARY_OP_ADD_UNICODE followed by STORE_FAST
        // where the store goes into the left argument.
        // So the inputs are the same as for all BINARY_OP
        // specializations, but there is no output.
        // At the end we just skip over the STORE_FAST.
        inst(BINARY_OP_INPLACE_ADD_UNICODE, (left, right --)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyUnicode_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            _Py_CODEUNIT true_next = next_instr[INLINE_CACHE_ENTRIES_BINARY_OP];
            assert(true_next.op.code == STORE_FAST ||
                   true_next.op.code == STORE_FAST__LOAD_FAST);
            PyObject **target_local = &GETLOCAL(true_next.op.arg);
            DEOPT_IF(*target_local != left, BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            /* Handle `left = left + right` or `left += right` for str.
             *
             * When possible, extend `left` in place rather than
             * allocating a new PyUnicodeObject. This attempts to avoid
             * quadratic behavior when one neglects to use str.join().
             *
             * If `left` has only two references remaining (one from
             * the stack, one in the locals), DECREFing `left` leaves
             * only the locals reference, so PyUnicode_Append knows
             * that the string is safe to mutate.
             */
            assert(Py_REFCNT(left) >= 2);
            _Py_DECREF_NO_DEALLOC(left);
            PyUnicode_Append(target_local, right);
            _Py_DECREF_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            ERROR_IF(*target_local == NULL, error);
            // The STORE_FAST is already done.
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_OP + 1);
        }

        inst(BINARY_OP_ADD_FLOAT, (unused/1, left, right -- sum)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyFloat_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            double dsum = ((PyFloatObject *)left)->ob_fval +
                ((PyFloatObject *)right)->ob_fval;
            sum = PyFloat_FromDouble(dsum);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            ERROR_IF(sum == NULL, error);
        }

        inst(BINARY_OP_ADD_INT, (unused/1, left, right -- sum)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyLong_CheckExact(left), BINARY_OP);
            DEOPT_IF(Py_TYPE(right) != Py_TYPE(left), BINARY_OP);
            STAT_INC(BINARY_OP, hit);
            sum = _PyLong_Add((PyLongObject *)left, (PyLongObject *)right);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            ERROR_IF(sum == NULL, error);
        }

        family(binary_subscr, INLINE_CACHE_ENTRIES_BINARY_SUBSCR) = {
            BINARY_SUBSCR,
            BINARY_SUBSCR_DICT,
            BINARY_SUBSCR_GETITEM,
            BINARY_SUBSCR_LIST_INT,
            BINARY_SUBSCR_TUPLE_INT,
        };

        inst(BINARY_SUBSCR, (unused/4, container, sub -- res)) {
            #if ENABLE_SPECIALIZATION
            _PyBinarySubscrCache *cache = (_PyBinarySubscrCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_BinarySubscr(container, sub, next_instr);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(BINARY_SUBSCR, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            res = PyObject_GetItem(container, sub);
            DECREF_INPUTS();
            ERROR_IF(res == NULL, error);
        }

        inst(BINARY_SLICE, (container, start, stop -- res)) {
            PyObject *slice = _PyBuildSlice_ConsumeRefs(start, stop);
            // Can't use ERROR_IF() here, because we haven't
            // DECREF'ed container yet, and we still own slice.
            if (slice == NULL) {
                res = NULL;
            }
            else {
                res = PyObject_GetItem(container, slice);
                Py_DECREF(slice);
            }
            Py_DECREF(container);
            ERROR_IF(res == NULL, error);
        }

        inst(STORE_SLICE, (v, container, start, stop -- )) {
            PyObject *slice = _PyBuildSlice_ConsumeRefs(start, stop);
            int err;
            if (slice == NULL) {
                err = 1;
            }
            else {
                err = PyObject_SetItem(container, slice, v);
                Py_DECREF(slice);
            }
            Py_DECREF(v);
            Py_DECREF(container);
            ERROR_IF(err, error);
        }

        inst(BINARY_SUBSCR_LIST_INT, (unused/4, list, sub -- res)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyLong_CheckExact(sub), BINARY_SUBSCR);
            DEOPT_IF(!PyList_CheckExact(list), BINARY_SUBSCR);

            // Deopt unless 0 <= sub < PyList_Size(list)
            DEOPT_IF(!_PyLong_IsPositiveSingleDigit(sub), BINARY_SUBSCR);
            assert(((PyLongObject *)_PyLong_GetZero())->long_value.ob_digit[0] == 0);
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(index >= PyList_GET_SIZE(list), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            res = PyList_GET_ITEM(list, index);
            assert(res != NULL);
            Py_INCREF(res);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            Py_DECREF(list);
        }

        inst(BINARY_SUBSCR_TUPLE_INT, (unused/4, tuple, sub -- res)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyLong_CheckExact(sub), BINARY_SUBSCR);
            DEOPT_IF(!PyTuple_CheckExact(tuple), BINARY_SUBSCR);

            // Deopt unless 0 <= sub < PyTuple_Size(list)
            DEOPT_IF(!_PyLong_IsPositiveSingleDigit(sub), BINARY_SUBSCR);
            assert(((PyLongObject *)_PyLong_GetZero())->long_value.ob_digit[0] == 0);
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            DEOPT_IF(index >= PyTuple_GET_SIZE(tuple), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            res = PyTuple_GET_ITEM(tuple, index);
            assert(res != NULL);
            Py_INCREF(res);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            Py_DECREF(tuple);
        }

        inst(BINARY_SUBSCR_DICT, (unused/4, dict, sub -- res)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(dict), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            res = PyDict_GetItemWithError(dict, sub);
            if (res == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_SetKeyError(sub);
                }
                Py_DECREF(dict);
                Py_DECREF(sub);
                ERROR_IF(true, error);
            }
            Py_INCREF(res);  // Do this before DECREF'ing dict, sub
            DECREF_INPUTS();
        }

        inst(BINARY_SUBSCR_GETITEM, (unused/1, type_version/2, func_version/1, container, sub -- unused)) {
            PyTypeObject *tp = Py_TYPE(container);
            DEOPT_IF(tp->tp_version_tag != type_version, BINARY_SUBSCR);
            assert(tp->tp_flags & Py_TPFLAGS_HEAPTYPE);
            PyObject *cached = ((PyHeapTypeObject *)tp)->_spec_cache.getitem;
            assert(PyFunction_Check(cached));
            PyFunctionObject *getitem = (PyFunctionObject *)cached;
            DEOPT_IF(getitem->func_version != func_version, BINARY_SUBSCR);
            PyCodeObject *code = (PyCodeObject *)getitem->func_code;
            assert(code->co_argcount == 2);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), BINARY_SUBSCR);
            STAT_INC(BINARY_SUBSCR, hit);
            Py_INCREF(getitem);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, getitem, 2);
            STACK_SHRINK(2);
            new_frame->localsplus[0] = container;
            new_frame->localsplus[1] = sub;
            JUMPBY(INLINE_CACHE_ENTRIES_BINARY_SUBSCR);
            DISPATCH_INLINED(new_frame);
        }

        inst(LIST_APPEND, (list, unused[oparg-1], v -- list, unused[oparg-1])) {
            ERROR_IF(_PyList_AppendTakeRef((PyListObject *)list, v) < 0, error);
            PREDICT(JUMP_BACKWARD);
        }

        inst(SET_ADD, (set, unused[oparg-1], v -- set, unused[oparg-1])) {
            int err = PySet_Add(set, v);
            Py_DECREF(v);
            ERROR_IF(err, error);
            PREDICT(JUMP_BACKWARD);
        }

        family(store_subscr, INLINE_CACHE_ENTRIES_STORE_SUBSCR) = {
            STORE_SUBSCR,
            STORE_SUBSCR_DICT,
            STORE_SUBSCR_LIST_INT,
        };

        inst(STORE_SUBSCR, (counter/1, v, container, sub -- )) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_IS_ZERO(counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_StoreSubscr(container, sub, next_instr);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(STORE_SUBSCR, deferred);
            _PyStoreSubscrCache *cache = (_PyStoreSubscrCache *)next_instr;
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #else
            (void)counter;  // Unused.
            #endif  /* ENABLE_SPECIALIZATION */
            /* container[sub] = v */
            int err = PyObject_SetItem(container, sub, v);
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        inst(STORE_SUBSCR_LIST_INT, (unused/1, value, list, sub -- )) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyLong_CheckExact(sub), STORE_SUBSCR);
            DEOPT_IF(!PyList_CheckExact(list), STORE_SUBSCR);

            // Ensure nonnegative, zero-or-one-digit ints.
            DEOPT_IF(!_PyLong_IsPositiveSingleDigit(sub), STORE_SUBSCR);
            Py_ssize_t index = ((PyLongObject*)sub)->long_value.ob_digit[0];
            // Ensure index < len(list)
            DEOPT_IF(index >= PyList_GET_SIZE(list), STORE_SUBSCR);
            STAT_INC(STORE_SUBSCR, hit);

            PyObject *old_value = PyList_GET_ITEM(list, index);
            PyList_SET_ITEM(list, index, value);
            assert(old_value != NULL);
            Py_DECREF(old_value);
            _Py_DECREF_SPECIALIZED(sub, (destructor)PyObject_Free);
            Py_DECREF(list);
        }

        inst(STORE_SUBSCR_DICT, (unused/1, value, dict, sub -- )) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(dict), STORE_SUBSCR);
            STAT_INC(STORE_SUBSCR, hit);
            int err = _PyDict_SetItem_Take2((PyDictObject *)dict, sub, value);
            Py_DECREF(dict);
            ERROR_IF(err, error);
        }

        inst(DELETE_SUBSCR, (container, sub --)) {
            /* del container[sub] */
            int err = PyObject_DelItem(container, sub);
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        inst(CALL_INTRINSIC_1, (value -- res)) {
            assert(oparg <= MAX_INTRINSIC_1);
            res = _PyIntrinsics_UnaryFunctions[oparg](tstate, value);
            DECREF_INPUTS();
            ERROR_IF(res == NULL, error);
        }

        inst(CALL_INTRINSIC_2, (value2, value1 -- res)) {
            assert(oparg <= MAX_INTRINSIC_2);
            res = _PyIntrinsics_BinaryFunctions[oparg](tstate, value2, value1);
            DECREF_INPUTS();
            ERROR_IF(res == NULL, error);
        }

        inst(RAISE_VARARGS, (args[oparg] -- )) {
            PyObject *cause = NULL, *exc = NULL;
            switch (oparg) {
            case 2:
                cause = args[1];
                /* fall through */
            case 1:
                exc = args[0];
                /* fall through */
            case 0:
                ERROR_IF(do_raise(tstate, exc, cause), exception_unwind);
                break;
            default:
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "bad RAISE_VARARGS oparg");
                break;
            }
            ERROR_IF(true, error);
        }

        inst(INTERPRETER_EXIT, (retval --)) {
            assert(frame == &entry_frame);
            assert(_PyFrame_IsIncomplete(frame));
            STACK_SHRINK(1);  // Since we're not going to DISPATCH()
            assert(EMPTY());
            /* Restore previous cframe and return. */
            tstate->cframe = cframe.previous;
            tstate->cframe->use_tracing = cframe.use_tracing;
            assert(tstate->cframe->current_frame == frame->previous);
            assert(!_PyErr_Occurred(tstate));
            _Py_LeaveRecursiveCallTstate(tstate);
            return retval;
        }

        inst(RETURN_VALUE, (retval --)) {
            STACK_SHRINK(1);
            assert(EMPTY());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            TRACE_FUNCTION_EXIT();
            DTRACE_FUNCTION_EXIT();
            _Py_LeaveRecursiveCallPy(tstate);
            assert(frame != &entry_frame);
            // GH-99729: We need to unlink the frame *before* clearing it:
            _PyInterpreterFrame *dying = frame;
            frame = cframe.current_frame = dying->previous;
            _PyEvalFrameClearAndPop(tstate, dying);
            _PyFrame_StackPush(frame, retval);
            goto resume_frame;
        }

        inst(RETURN_CONST, (--)) {
            PyObject *retval = GETITEM(consts, oparg);
            Py_INCREF(retval);
            assert(EMPTY());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            TRACE_FUNCTION_EXIT();
            DTRACE_FUNCTION_EXIT();
            _Py_LeaveRecursiveCallPy(tstate);
            assert(frame != &entry_frame);
            // GH-99729: We need to unlink the frame *before* clearing it:
            _PyInterpreterFrame *dying = frame;
            frame = cframe.current_frame = dying->previous;
            _PyEvalFrameClearAndPop(tstate, dying);
            _PyFrame_StackPush(frame, retval);
            goto resume_frame;
        }

        inst(GET_AITER, (obj -- iter)) {
            unaryfunc getter = NULL;
            PyTypeObject *type = Py_TYPE(obj);

            if (type->tp_as_async != NULL) {
                getter = type->tp_as_async->am_aiter;
            }

            if (getter == NULL) {
                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' requires an object with "
                              "__aiter__ method, got %.100s",
                              type->tp_name);
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }

            iter = (*getter)(obj);
            DECREF_INPUTS();
            ERROR_IF(iter == NULL, error);

            if (Py_TYPE(iter)->tp_as_async == NULL ||
                    Py_TYPE(iter)->tp_as_async->am_anext == NULL) {

                _PyErr_Format(tstate, PyExc_TypeError,
                              "'async for' received an object from __aiter__ "
                              "that does not implement __anext__: %.100s",
                              Py_TYPE(iter)->tp_name);
                Py_DECREF(iter);
                ERROR_IF(true, error);
            }
        }

        inst(GET_ANEXT, (aiter -- aiter, awaitable)) {
            unaryfunc getter = NULL;
            PyObject *next_iter = NULL;
            PyTypeObject *type = Py_TYPE(aiter);

            if (PyAsyncGen_CheckExact(aiter)) {
                awaitable = type->tp_as_async->am_anext(aiter);
                if (awaitable == NULL) {
                    goto error;
                }
            } else {
                if (type->tp_as_async != NULL){
                    getter = type->tp_as_async->am_anext;
                }

                if (getter != NULL) {
                    next_iter = (*getter)(aiter);
                    if (next_iter == NULL) {
                        goto error;
                    }
                }
                else {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'async for' requires an iterator with "
                                  "__anext__ method, got %.100s",
                                  type->tp_name);
                    goto error;
                }

                awaitable = _PyCoro_GetAwaitableIter(next_iter);
                if (awaitable == NULL) {
                    _PyErr_FormatFromCause(
                        PyExc_TypeError,
                        "'async for' received an invalid object "
                        "from __anext__: %.100s",
                        Py_TYPE(next_iter)->tp_name);

                    Py_DECREF(next_iter);
                    goto error;
                } else {
                    Py_DECREF(next_iter);
                }
            }

            PREDICT(LOAD_CONST);
        }

        inst(GET_AWAITABLE, (iterable -- iter)) {
            iter = _PyCoro_GetAwaitableIter(iterable);

            if (iter == NULL) {
                format_awaitable_error(tstate, Py_TYPE(iterable), oparg);
            }

            DECREF_INPUTS();

            if (iter != NULL && PyCoro_CheckExact(iter)) {
                PyObject *yf = _PyGen_yf((PyGenObject*)iter);
                if (yf != NULL) {
                    /* `iter` is a coroutine object that is being
                       awaited, `yf` is a pointer to the current awaitable
                       being awaited on. */
                    Py_DECREF(yf);
                    Py_CLEAR(iter);
                    _PyErr_SetString(tstate, PyExc_RuntimeError,
                                     "coroutine is being awaited already");
                    /* The code below jumps to `error` if `iter` is NULL. */
                }
            }

            ERROR_IF(iter == NULL, error);

            PREDICT(LOAD_CONST);
        }

        family(for_iter, INLINE_CACHE_ENTRIES_FOR_ITER) = {
            SEND,
            SEND_GEN,
        };

        inst(SEND, (unused/1, receiver, v -- receiver, retval)) {
            #if ENABLE_SPECIALIZATION
            _PySendCache *cache = (_PySendCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_Send(receiver, next_instr);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(SEND, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            assert(frame != &entry_frame);
            if (Py_IsNone(v) && PyIter_Check(receiver)) {
                retval = Py_TYPE(receiver)->tp_iternext(receiver);
            }
            else {
                retval = PyObject_CallMethodOneArg(receiver, &_Py_ID(send), v);
            }
            if (retval == NULL) {
                if (tstate->c_tracefunc != NULL
                        && _PyErr_ExceptionMatches(tstate, PyExc_StopIteration))
                    call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, frame);
                if (_PyGen_FetchStopIterationValue(&retval) == 0) {
                    assert(retval != NULL);
                    JUMPBY(oparg);
                }
                else {
                    assert(retval == NULL);
                    goto error;
                }
            }
            else {
                assert(retval != NULL);
            }
            Py_DECREF(v);
        }

        inst(SEND_GEN, (unused/1, receiver, v -- receiver)) {
            assert(cframe.use_tracing == 0);
            PyGenObject *gen = (PyGenObject *)receiver;
            DEOPT_IF(Py_TYPE(gen) != &PyGen_Type &&
                     Py_TYPE(gen) != &PyCoro_Type, SEND);
            DEOPT_IF(gen->gi_frame_state >= FRAME_EXECUTING, SEND);
            STAT_INC(SEND, hit);
            _PyInterpreterFrame *gen_frame = (_PyInterpreterFrame *)gen->gi_iframe;
            frame->yield_offset = oparg;
            STACK_SHRINK(1);
            _PyFrame_StackPush(gen_frame, v);
            gen->gi_frame_state = FRAME_EXECUTING;
            gen->gi_exc_state.previous_item = tstate->exc_info;
            tstate->exc_info = &gen->gi_exc_state;
            JUMPBY(INLINE_CACHE_ENTRIES_SEND + oparg);
            DISPATCH_INLINED(gen_frame);
        }

        inst(YIELD_VALUE, (retval -- unused)) {
            // NOTE: It's important that YIELD_VALUE never raises an exception!
            // The compiler treats any exception raised here as a failed close()
            // or throw() call.
            assert(frame != &entry_frame);
            PyGenObject *gen = _PyFrame_GetGenerator(frame);
            gen->gi_frame_state = FRAME_SUSPENDED;
            _PyFrame_SetStackPointer(frame, stack_pointer - 1);
            TRACE_FUNCTION_EXIT();
            DTRACE_FUNCTION_EXIT();
            tstate->exc_info = gen->gi_exc_state.previous_item;
            gen->gi_exc_state.previous_item = NULL;
            _Py_LeaveRecursiveCallPy(tstate);
            _PyInterpreterFrame *gen_frame = frame;
            frame = cframe.current_frame = frame->previous;
            gen_frame->previous = NULL;
            frame->prev_instr -= frame->yield_offset;
            _PyFrame_StackPush(frame, retval);
            goto resume_frame;
        }

        inst(POP_EXCEPT, (exc_value -- )) {
            _PyErr_StackItem *exc_info = tstate->exc_info;
            Py_XSETREF(exc_info->exc_value, exc_value);
        }

        inst(RERAISE, (values[oparg], exc -- values[oparg])) {
            assert(oparg >= 0 && oparg <= 2);
            if (oparg) {
                PyObject *lasti = values[0];
                if (PyLong_Check(lasti)) {
                    frame->prev_instr = _PyCode_CODE(frame->f_code) + PyLong_AsLong(lasti);
                    assert(!_PyErr_Occurred(tstate));
                }
                else {
                    assert(PyLong_Check(lasti));
                    _PyErr_SetString(tstate, PyExc_SystemError, "lasti is not an int");
                    goto error;
                }
            }
            assert(exc && PyExceptionInstance_Check(exc));
            Py_INCREF(exc);
            PyObject *typ = Py_NewRef(PyExceptionInstance_Class(exc));
            PyObject *tb = PyException_GetTraceback(exc);
            _PyErr_Restore(tstate, typ, exc, tb);
            goto exception_unwind;
        }

        inst(END_ASYNC_FOR, (awaitable, exc -- )) {
            assert(exc && PyExceptionInstance_Check(exc));
            if (PyErr_GivenExceptionMatches(exc, PyExc_StopAsyncIteration)) {
                DECREF_INPUTS();
            }
            else {
                Py_INCREF(exc);
                PyObject *typ = Py_NewRef(PyExceptionInstance_Class(exc));
                PyObject *tb = PyException_GetTraceback(exc);
                _PyErr_Restore(tstate, typ, exc, tb);
                goto exception_unwind;
            }
        }

        inst(CLEANUP_THROW, (sub_iter, last_sent_val, exc_value -- none, value)) {
            assert(throwflag);
            assert(exc_value && PyExceptionInstance_Check(exc_value));
            if (PyErr_GivenExceptionMatches(exc_value, PyExc_StopIteration)) {
                value = Py_NewRef(((PyStopIterationObject *)exc_value)->value);
                DECREF_INPUTS();
                none = Py_NewRef(Py_None);
            }
            else {
                _PyErr_SetRaisedException(tstate, Py_NewRef(exc_value));
                goto exception_unwind;
            }
        }

        inst(LOAD_ASSERTION_ERROR, ( -- value)) {
            value = Py_NewRef(PyExc_AssertionError);
        }

        inst(LOAD_BUILD_CLASS, ( -- bc)) {
            if (PyDict_CheckExact(BUILTINS())) {
                bc = _PyDict_GetItemWithError(BUILTINS(),
                                              &_Py_ID(__build_class__));
                if (bc == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        _PyErr_SetString(tstate, PyExc_NameError,
                                         "__build_class__ not found");
                    }
                    ERROR_IF(true, error);
                }
                Py_INCREF(bc);
            }
            else {
                bc = PyObject_GetItem(BUILTINS(), &_Py_ID(__build_class__));
                if (bc == NULL) {
                    if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                        _PyErr_SetString(tstate, PyExc_NameError,
                                         "__build_class__ not found");
                    ERROR_IF(true, error);
                }
            }
        }

        inst(STORE_NAME, (v -- )) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when storing %R", name);
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            if (PyDict_CheckExact(ns))
                err = PyDict_SetItem(ns, name, v);
            else
                err = PyObject_SetItem(ns, name, v);
            DECREF_INPUTS();
            ERROR_IF(err, error);
        }

        inst(DELETE_NAME, (--)) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *ns = LOCALS();
            int err;
            if (ns == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when deleting %R", name);
                goto error;
            }
            err = PyObject_DelItem(ns, name);
            // Can't use ERROR_IF here.
            if (err != 0) {
                format_exc_check_arg(tstate, PyExc_NameError,
                                     NAME_ERROR_MSG,
                                     name);
                goto error;
            }
        }

        family(unpack_sequence, INLINE_CACHE_ENTRIES_UNPACK_SEQUENCE) = {
            UNPACK_SEQUENCE,
            UNPACK_SEQUENCE_TWO_TUPLE,
            UNPACK_SEQUENCE_TUPLE,
            UNPACK_SEQUENCE_LIST,
        };

        inst(UNPACK_SEQUENCE, (unused/1, seq -- unused[oparg])) {
            #if ENABLE_SPECIALIZATION
            _PyUnpackSequenceCache *cache = (_PyUnpackSequenceCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_UnpackSequence(seq, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(UNPACK_SEQUENCE, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            PyObject **top = stack_pointer + oparg - 1;
            int res = unpack_iterable(tstate, seq, oparg, -1, top);
            Py_DECREF(seq);
            ERROR_IF(res == 0, error);
        }

        inst(UNPACK_SEQUENCE_TWO_TUPLE, (unused/1, seq -- values[oparg])) {
            DEOPT_IF(!PyTuple_CheckExact(seq), UNPACK_SEQUENCE);
            DEOPT_IF(PyTuple_GET_SIZE(seq) != 2, UNPACK_SEQUENCE);
            assert(oparg == 2);
            STAT_INC(UNPACK_SEQUENCE, hit);
            values[0] = Py_NewRef(PyTuple_GET_ITEM(seq, 1));
            values[1] = Py_NewRef(PyTuple_GET_ITEM(seq, 0));
            Py_DECREF(seq);
        }

        inst(UNPACK_SEQUENCE_TUPLE, (unused/1, seq -- values[oparg])) {
            DEOPT_IF(!PyTuple_CheckExact(seq), UNPACK_SEQUENCE);
            DEOPT_IF(PyTuple_GET_SIZE(seq) != oparg, UNPACK_SEQUENCE);
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyObject **items = _PyTuple_ITEMS(seq);
            for (int i = oparg; --i >= 0; ) {
                *values++ = Py_NewRef(items[i]);
            }
            Py_DECREF(seq);
        }

        inst(UNPACK_SEQUENCE_LIST, (unused/1, seq -- values[oparg])) {
            DEOPT_IF(!PyList_CheckExact(seq), UNPACK_SEQUENCE);
            DEOPT_IF(PyList_GET_SIZE(seq) != oparg, UNPACK_SEQUENCE);
            STAT_INC(UNPACK_SEQUENCE, hit);
            PyObject **items = _PyList_ITEMS(seq);
            for (int i = oparg; --i >= 0; ) {
                *values++ = Py_NewRef(items[i]);
            }
            Py_DECREF(seq);
        }

        inst(UNPACK_EX, (seq -- unused[oparg & 0xFF], unused, unused[oparg >> 8])) {
            int totalargs = 1 + (oparg & 0xFF) + (oparg >> 8);
            PyObject **top = stack_pointer + totalargs - 1;
            int res = unpack_iterable(tstate, seq, oparg & 0xFF, oparg >> 8, top);
            Py_DECREF(seq);
            ERROR_IF(res == 0, error);
        }

        family(store_attr, INLINE_CACHE_ENTRIES_STORE_ATTR) = {
            STORE_ATTR,
            STORE_ATTR_INSTANCE_VALUE,
            STORE_ATTR_SLOT,
            STORE_ATTR_WITH_HINT,
        };

        inst(STORE_ATTR, (counter/1, unused/3, v, owner --)) {
            #if ENABLE_SPECIALIZATION
            if (ADAPTIVE_COUNTER_IS_ZERO(counter)) {
                assert(cframe.use_tracing == 0);
                PyObject *name = GETITEM(names, oparg);
                next_instr--;
                _Py_Specialize_StoreAttr(owner, next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(STORE_ATTR, deferred);
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #else
            (void)counter;  // Unused.
            #endif  /* ENABLE_SPECIALIZATION */
            PyObject *name = GETITEM(names, oparg);
            int err = PyObject_SetAttr(owner, name, v);
            Py_DECREF(v);
            Py_DECREF(owner);
            ERROR_IF(err, error);
        }

        inst(DELETE_ATTR, (owner --)) {
            PyObject *name = GETITEM(names, oparg);
            int err = PyObject_SetAttr(owner, name, (PyObject *)NULL);
            Py_DECREF(owner);
            ERROR_IF(err, error);
        }

        inst(STORE_GLOBAL, (v --)) {
            PyObject *name = GETITEM(names, oparg);
            int err = PyDict_SetItem(GLOBALS(), name, v);
            Py_DECREF(v);
            ERROR_IF(err, error);
        }

        inst(DELETE_GLOBAL, (--)) {
            PyObject *name = GETITEM(names, oparg);
            int err;
            err = PyDict_DelItem(GLOBALS(), name);
            // Can't use ERROR_IF here.
            if (err != 0) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                    format_exc_check_arg(tstate, PyExc_NameError,
                                         NAME_ERROR_MSG, name);
                }
                goto error;
            }
        }

        inst(LOAD_NAME, ( -- v)) {
            PyObject *name = GETITEM(names, oparg);
            PyObject *locals = LOCALS();
            if (locals == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals when loading %R", name);
                goto error;
            }
            if (PyDict_CheckExact(locals)) {
                v = PyDict_GetItemWithError(locals, name);
                if (v != NULL) {
                    Py_INCREF(v);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            }
            else {
                v = PyObject_GetItem(locals, name);
                if (v == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError))
                        goto error;
                    _PyErr_Clear(tstate);
                }
            }
            if (v == NULL) {
                v = PyDict_GetItemWithError(GLOBALS(), name);
                if (v != NULL) {
                    Py_INCREF(v);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
                else {
                    if (PyDict_CheckExact(BUILTINS())) {
                        v = PyDict_GetItemWithError(BUILTINS(), name);
                        if (v == NULL) {
                            if (!_PyErr_Occurred(tstate)) {
                                format_exc_check_arg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                            }
                            goto error;
                        }
                        Py_INCREF(v);
                    }
                    else {
                        v = PyObject_GetItem(BUILTINS(), name);
                        if (v == NULL) {
                            if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                                format_exc_check_arg(
                                            tstate, PyExc_NameError,
                                            NAME_ERROR_MSG, name);
                            }
                            goto error;
                        }
                    }
                }
            }
        }

        family(load_global, INLINE_CACHE_ENTRIES_LOAD_GLOBAL) = {
            LOAD_GLOBAL,
            LOAD_GLOBAL_MODULE,
            LOAD_GLOBAL_BUILTIN,
        };

        inst(LOAD_GLOBAL, (unused/1, unused/1, unused/2, unused/1 -- null if (oparg & 1), v)) {
            #if ENABLE_SPECIALIZATION
            _PyLoadGlobalCache *cache = (_PyLoadGlobalCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                PyObject *name = GETITEM(names, oparg>>1);
                next_instr--;
                _Py_Specialize_LoadGlobal(GLOBALS(), BUILTINS(), next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(LOAD_GLOBAL, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            PyObject *name = GETITEM(names, oparg>>1);
            if (PyDict_CheckExact(GLOBALS())
                && PyDict_CheckExact(BUILTINS()))
            {
                v = _PyDict_LoadGlobal((PyDictObject *)GLOBALS(),
                                       (PyDictObject *)BUILTINS(),
                                       name);
                if (v == NULL) {
                    if (!_PyErr_Occurred(tstate)) {
                        /* _PyDict_LoadGlobal() returns NULL without raising
                         * an exception if the key doesn't exist */
                        format_exc_check_arg(tstate, PyExc_NameError,
                                             NAME_ERROR_MSG, name);
                    }
                    ERROR_IF(true, error);
                }
                Py_INCREF(v);
            }
            else {
                /* Slow-path if globals or builtins is not a dict */

                /* namespace 1: globals */
                v = PyObject_GetItem(GLOBALS(), name);
                if (v == NULL) {
                    ERROR_IF(!_PyErr_ExceptionMatches(tstate, PyExc_KeyError), error);
                    _PyErr_Clear(tstate);

                    /* namespace 2: builtins */
                    v = PyObject_GetItem(BUILTINS(), name);
                    if (v == NULL) {
                        if (_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                            format_exc_check_arg(
                                        tstate, PyExc_NameError,
                                        NAME_ERROR_MSG, name);
                        }
                        ERROR_IF(true, error);
                    }
                }
            }
            null = NULL;
        }

        inst(LOAD_GLOBAL_MODULE, (unused/1, index/1, version/2, unused/1 -- null if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(GLOBALS()), LOAD_GLOBAL);
            PyDictObject *dict = (PyDictObject *)GLOBALS();
            DEOPT_IF(dict->ma_keys->dk_version != version, LOAD_GLOBAL);
            assert(DK_IS_UNICODE(dict->ma_keys));
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dict->ma_keys);
            res = entries[index].me_value;
            DEOPT_IF(res == NULL, LOAD_GLOBAL);
            Py_INCREF(res);
            STAT_INC(LOAD_GLOBAL, hit);
            null = NULL;
        }

        inst(LOAD_GLOBAL_BUILTIN, (unused/1, index/1, mod_version/2, bltn_version/1 -- null if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyDict_CheckExact(GLOBALS()), LOAD_GLOBAL);
            DEOPT_IF(!PyDict_CheckExact(BUILTINS()), LOAD_GLOBAL);
            PyDictObject *mdict = (PyDictObject *)GLOBALS();
            PyDictObject *bdict = (PyDictObject *)BUILTINS();
            DEOPT_IF(mdict->ma_keys->dk_version != mod_version, LOAD_GLOBAL);
            DEOPT_IF(bdict->ma_keys->dk_version != bltn_version, LOAD_GLOBAL);
            assert(DK_IS_UNICODE(bdict->ma_keys));
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(bdict->ma_keys);
            res = entries[index].me_value;
            DEOPT_IF(res == NULL, LOAD_GLOBAL);
            Py_INCREF(res);
            STAT_INC(LOAD_GLOBAL, hit);
            null = NULL;
        }

        inst(DELETE_FAST, (--)) {
            PyObject *v = GETLOCAL(oparg);
            ERROR_IF(v == NULL, unbound_local_error);
            SETLOCAL(oparg, NULL);
        }

        inst(MAKE_CELL, (--)) {
            // "initial" is probably NULL but not if it's an arg (or set
            // via PyFrame_LocalsToFast() before MAKE_CELL has run).
            PyObject *initial = GETLOCAL(oparg);
            PyObject *cell = PyCell_New(initial);
            if (cell == NULL) {
                goto resume_with_error;
            }
            SETLOCAL(oparg, cell);
        }

        inst(DELETE_DEREF, (--)) {
            PyObject *cell = GETLOCAL(oparg);
            PyObject *oldobj = PyCell_GET(cell);
            // Can't use ERROR_IF here.
            // Fortunately we don't need its superpower.
            if (oldobj == NULL) {
                format_exc_unbound(tstate, frame->f_code, oparg);
                goto error;
            }
            PyCell_SET(cell, NULL);
            Py_DECREF(oldobj);
        }

        inst(LOAD_CLASSDEREF, ( -- value)) {
            PyObject *name, *locals = LOCALS();
            assert(locals);
            assert(oparg >= 0 && oparg < frame->f_code->co_nlocalsplus);
            name = PyTuple_GET_ITEM(frame->f_code->co_localsplusnames, oparg);
            if (PyDict_CheckExact(locals)) {
                value = PyDict_GetItemWithError(locals, name);
                if (value != NULL) {
                    Py_INCREF(value);
                }
                else if (_PyErr_Occurred(tstate)) {
                    goto error;
                }
            }
            else {
                value = PyObject_GetItem(locals, name);
                if (value == NULL) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_KeyError)) {
                        goto error;
                    }
                    _PyErr_Clear(tstate);
                }
            }
            if (!value) {
                PyObject *cell = GETLOCAL(oparg);
                value = PyCell_GET(cell);
                if (value == NULL) {
                    format_exc_unbound(tstate, frame->f_code, oparg);
                    goto error;
                }
                Py_INCREF(value);
            }
        }

        inst(LOAD_DEREF, ( -- value)) {
            PyObject *cell = GETLOCAL(oparg);
            value = PyCell_GET(cell);
            if (value == NULL) {
                format_exc_unbound(tstate, frame->f_code, oparg);
                ERROR_IF(true, error);
            }
            Py_INCREF(value);
        }

        inst(STORE_DEREF, (v --)) {
            PyObject *cell = GETLOCAL(oparg);
            PyObject *oldobj = PyCell_GET(cell);
            PyCell_SET(cell, v);
            Py_XDECREF(oldobj);
        }

        inst(COPY_FREE_VARS, (--)) {
            /* Copy closure variables to free variables */
            PyCodeObject *co = frame->f_code;
            assert(PyFunction_Check(frame->f_funcobj));
            PyObject *closure = ((PyFunctionObject *)frame->f_funcobj)->func_closure;
            assert(oparg == co->co_nfreevars);
            int offset = co->co_nlocalsplus - oparg;
            for (int i = 0; i < oparg; ++i) {
                PyObject *o = PyTuple_GET_ITEM(closure, i);
                frame->localsplus[offset + i] = Py_NewRef(o);
            }
        }

        inst(BUILD_STRING, (pieces[oparg] -- str)) {
            str = _PyUnicode_JoinArray(&_Py_STR(empty), pieces, oparg);
            for (int i = 0; i < oparg; i++) {
                Py_DECREF(pieces[i]);
            }
            ERROR_IF(str == NULL, error);
        }

        inst(BUILD_TUPLE, (values[oparg] -- tup)) {
            tup = _PyTuple_FromArraySteal(values, oparg);
            ERROR_IF(tup == NULL, error);
        }

        inst(BUILD_LIST, (values[oparg] -- list)) {
            list = _PyList_FromArraySteal(values, oparg);
            ERROR_IF(list == NULL, error);
        }

        inst(LIST_EXTEND, (list, unused[oparg-1], iterable -- list, unused[oparg-1])) {
            PyObject *none_val = _PyList_Extend((PyListObject *)list, iterable);
            if (none_val == NULL) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError) &&
                   (Py_TYPE(iterable)->tp_iter == NULL && !PySequence_Check(iterable)))
                {
                    _PyErr_Clear(tstate);
                    _PyErr_Format(tstate, PyExc_TypeError,
                          "Value after * must be an iterable, not %.200s",
                          Py_TYPE(iterable)->tp_name);
                }
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            Py_DECREF(none_val);
            DECREF_INPUTS();
        }

        inst(SET_UPDATE, (set, unused[oparg-1], iterable -- set, unused[oparg-1])) {
            int err = _PySet_Update(set, iterable);
            DECREF_INPUTS();
            ERROR_IF(err < 0, error);
        }

        inst(BUILD_SET, (values[oparg] -- set)) {
            set = PySet_New(NULL);
            if (set == NULL)
                goto error;
            int err = 0;
            for (int i = 0; i < oparg; i++) {
                PyObject *item = values[i];
                if (err == 0)
                    err = PySet_Add(set, item);
                Py_DECREF(item);
            }
            if (err != 0) {
                Py_DECREF(set);
                ERROR_IF(true, error);
            }
        }

        inst(BUILD_MAP, (values[oparg*2] -- map)) {
            map = _PyDict_FromItems(
                    values, 2,
                    values+1, 2,
                    oparg);
            if (map == NULL)
                goto error;

            for (int i = 0; i < oparg; i++) {
                Py_DECREF(values[i*2]);
                Py_DECREF(values[i*2+1]);
            }
            ERROR_IF(map == NULL, error);
        }

        inst(SETUP_ANNOTATIONS, (--)) {
            int err;
            PyObject *ann_dict;
            if (LOCALS() == NULL) {
                _PyErr_Format(tstate, PyExc_SystemError,
                              "no locals found when setting up annotations");
                ERROR_IF(true, error);
            }
            /* check if __annotations__ in locals()... */
            if (PyDict_CheckExact(LOCALS())) {
                ann_dict = _PyDict_GetItemWithError(LOCALS(),
                                                    &_Py_ID(__annotations__));
                if (ann_dict == NULL) {
                    ERROR_IF(_PyErr_Occurred(tstate), error);
                    /* ...if not, create a new one */
                    ann_dict = PyDict_New();
                    ERROR_IF(ann_dict == NULL, error);
                    err = PyDict_SetItem(LOCALS(), &_Py_ID(__annotations__),
                                         ann_dict);
                    Py_DECREF(ann_dict);
                    ERROR_IF(err, error);
                }
            }
            else {
                /* do the same if locals() is not a dict */
                ann_dict = PyObject_GetItem(LOCALS(), &_Py_ID(__annotations__));
                if (ann_dict == NULL) {
                    ERROR_IF(!_PyErr_ExceptionMatches(tstate, PyExc_KeyError), error);
                    _PyErr_Clear(tstate);
                    ann_dict = PyDict_New();
                    ERROR_IF(ann_dict == NULL, error);
                    err = PyObject_SetItem(LOCALS(), &_Py_ID(__annotations__),
                                           ann_dict);
                    Py_DECREF(ann_dict);
                    ERROR_IF(err, error);
                }
                else {
                    Py_DECREF(ann_dict);
                }
            }
        }

        inst(BUILD_CONST_KEY_MAP, (values[oparg], keys -- map)) {
            if (!PyTuple_CheckExact(keys) ||
                PyTuple_GET_SIZE(keys) != (Py_ssize_t)oparg) {
                _PyErr_SetString(tstate, PyExc_SystemError,
                                 "bad BUILD_CONST_KEY_MAP keys argument");
                goto error;  // Pop the keys and values.
            }
            map = _PyDict_FromItems(
                    &PyTuple_GET_ITEM(keys, 0), 1,
                    values, 1, oparg);
            Py_DECREF(keys);
            for (int i = 0; i < oparg; i++) {
                Py_DECREF(values[i]);
            }
            ERROR_IF(map == NULL, error);
        }

        inst(DICT_UPDATE, (update --)) {
            PyObject *dict = PEEK(oparg + 1);  // update is still on the stack
            if (PyDict_Update(dict, update) < 0) {
                if (_PyErr_ExceptionMatches(tstate, PyExc_AttributeError)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                    "'%.200s' object is not a mapping",
                                    Py_TYPE(update)->tp_name);
                }
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            DECREF_INPUTS();
        }

        inst(DICT_MERGE, (update --)) {
            PyObject *dict = PEEK(oparg + 1);  // update is still on the stack

            if (_PyDict_MergeEx(dict, update, 2) < 0) {
                format_kwargs_error(tstate, PEEK(3 + oparg), update);
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }
            DECREF_INPUTS();
            PREDICT(CALL_FUNCTION_EX);
        }

        inst(MAP_ADD, (key, value --)) {
            PyObject *dict = PEEK(oparg + 2);  // key, value are still on the stack
            assert(PyDict_CheckExact(dict));
            /* dict[key] = value */
            // Do not DECREF INPUTS because the function steals the references
            ERROR_IF(_PyDict_SetItem_Take2((PyDictObject *)dict, key, value) != 0, error);
            PREDICT(JUMP_BACKWARD);
        }

        family(load_attr, INLINE_CACHE_ENTRIES_LOAD_ATTR) = {
            LOAD_ATTR,
            LOAD_ATTR_INSTANCE_VALUE,
            LOAD_ATTR_MODULE,
            LOAD_ATTR_WITH_HINT,
            LOAD_ATTR_SLOT,
            LOAD_ATTR_CLASS,
            LOAD_ATTR_PROPERTY,
            LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN,
            LOAD_ATTR_METHOD_WITH_VALUES,
            LOAD_ATTR_METHOD_NO_DICT,
            LOAD_ATTR_METHOD_LAZY_DICT,
        };

        inst(LOAD_ATTR, (unused/9, owner -- res2 if (oparg & 1), res)) {
            #if ENABLE_SPECIALIZATION
            _PyAttrCache *cache = (_PyAttrCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                PyObject *name = GETITEM(names, oparg>>1);
                next_instr--;
                _Py_Specialize_LoadAttr(owner, next_instr, name);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(LOAD_ATTR, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            PyObject *name = GETITEM(names, oparg >> 1);
            if (oparg & 1) {
                /* Designed to work in tandem with CALL, pushes two values. */
                PyObject* meth = NULL;
                if (_PyObject_GetMethod(owner, name, &meth)) {
                    /* We can bypass temporary bound method object.
                       meth is unbound method and obj is self.

                       meth | self | arg1 | ... | argN
                     */
                    assert(meth != NULL);  // No errors on this branch
                    res2 = meth;
                    res = owner;  // Transfer ownership
                }
                else {
                    /* meth is not an unbound method (but a regular attr, or
                       something was returned by a descriptor protocol).  Set
                       the second element of the stack to NULL, to signal
                       CALL that it's not a method call.

                       NULL | meth | arg1 | ... | argN
                    */
                    Py_DECREF(owner);
                    ERROR_IF(meth == NULL, error);
                    res2 = NULL;
                    res = meth;
                }
            }
            else {
                /* Classic, pushes one value. */
                res = PyObject_GetAttr(owner, name);
                Py_DECREF(owner);
                ERROR_IF(res == NULL, error);
            }
        }

        inst(LOAD_ATTR_INSTANCE_VALUE, (unused/1, type_version/2, index/1, unused/5, owner -- res2 if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *tp = Py_TYPE(owner);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, LOAD_ATTR);
            assert(tp->tp_dictoffset < 0);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(!_PyDictOrValues_IsValues(dorv), LOAD_ATTR);
            res = _PyDictOrValues_GetValues(dorv)->values[index];
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            res2 = NULL;
            Py_DECREF(owner);
        }

        inst(LOAD_ATTR_MODULE, (unused/1, type_version/2, index/1, unused/5, owner -- res2 if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyModule_CheckExact(owner), LOAD_ATTR);
            PyDictObject *dict = (PyDictObject *)((PyModuleObject *)owner)->md_dict;
            assert(dict != NULL);
            DEOPT_IF(dict->ma_keys->dk_version != type_version, LOAD_ATTR);
            assert(dict->ma_keys->dk_kind == DICT_KEYS_UNICODE);
            assert(index < dict->ma_keys->dk_nentries);
            PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + index;
            res = ep->me_value;
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            res2 = NULL;
            Py_DECREF(owner);
        }

        inst(LOAD_ATTR_WITH_HINT, (unused/1, type_version/2, index/1, unused/5, owner -- res2 if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *tp = Py_TYPE(owner);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, LOAD_ATTR);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(_PyDictOrValues_IsValues(dorv), LOAD_ATTR);
            PyDictObject *dict = (PyDictObject *)_PyDictOrValues_GetDict(dorv);
            DEOPT_IF(dict == NULL, LOAD_ATTR);
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(names, oparg>>1);
            uint16_t hint = index;
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries, LOAD_ATTR);
            if (DK_IS_UNICODE(dict->ma_keys)) {
                PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, LOAD_ATTR);
                res = ep->me_value;
            }
            else {
                PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, LOAD_ATTR);
                res = ep->me_value;
            }
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            res2 = NULL;
            Py_DECREF(owner);
        }

        inst(LOAD_ATTR_SLOT, (unused/1, type_version/2, index/1, unused/5, owner -- res2 if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *tp = Py_TYPE(owner);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, LOAD_ATTR);
            char *addr = (char *)owner + index;
            res = *(PyObject **)addr;
            DEOPT_IF(res == NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(res);
            res2 = NULL;
            Py_DECREF(owner);
        }

        inst(LOAD_ATTR_CLASS, (unused/1, type_version/2, unused/2, descr/4, cls -- res2 if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);

            DEOPT_IF(!PyType_Check(cls), LOAD_ATTR);
            DEOPT_IF(((PyTypeObject *)cls)->tp_version_tag != type_version,
                LOAD_ATTR);
            assert(type_version != 0);

            STAT_INC(LOAD_ATTR, hit);
            res2 = NULL;
            res = descr;
            assert(res != NULL);
            Py_INCREF(res);
            Py_DECREF(cls);
        }

        inst(LOAD_ATTR_PROPERTY, (unused/1, type_version/2, func_version/2, fget/4, owner -- unused if (oparg & 1), unused)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(tstate->interp->eval_frame, LOAD_ATTR);

            PyTypeObject *cls = Py_TYPE(owner);
            DEOPT_IF(cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(type_version != 0);
            assert(Py_IS_TYPE(fget, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)fget;
            assert(func_version != 0);
            DEOPT_IF(f->func_version != func_version, LOAD_ATTR);
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            assert(code->co_argcount == 1);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            Py_INCREF(fget);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, f, 1);
            // Manipulate stack directly because we exit with DISPATCH_INLINED().
            SET_TOP(NULL);
            int shrink_stack = !(oparg & 1);
            STACK_SHRINK(shrink_stack);
            new_frame->localsplus[0] = owner;
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
            DISPATCH_INLINED(new_frame);
        }

        inst(LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN, (unused/1, type_version/2, func_version/2, getattribute/4, owner -- unused if (oparg & 1), unused)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(tstate->interp->eval_frame, LOAD_ATTR);
            PyTypeObject *cls = Py_TYPE(owner);
            DEOPT_IF(cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(type_version != 0);
            assert(Py_IS_TYPE(getattribute, &PyFunction_Type));
            PyFunctionObject *f = (PyFunctionObject *)getattribute;
            assert(func_version != 0);
            DEOPT_IF(f->func_version != func_version, LOAD_ATTR);
            PyCodeObject *code = (PyCodeObject *)f->func_code;
            assert(code->co_argcount == 2);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);

            PyObject *name = GETITEM(names, oparg >> 1);
            Py_INCREF(f);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, f, 2);
            // Manipulate stack directly because we exit with DISPATCH_INLINED().
            SET_TOP(NULL);
            int shrink_stack = !(oparg & 1);
            STACK_SHRINK(shrink_stack);
            new_frame->localsplus[0] = owner;
            new_frame->localsplus[1] = Py_NewRef(name);
            JUMPBY(INLINE_CACHE_ENTRIES_LOAD_ATTR);
            DISPATCH_INLINED(new_frame);
        }

        inst(STORE_ATTR_INSTANCE_VALUE, (unused/1, type_version/2, index/1, value, owner --)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *tp = Py_TYPE(owner);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, STORE_ATTR);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(!_PyDictOrValues_IsValues(dorv), STORE_ATTR);
            STAT_INC(STORE_ATTR, hit);
            PyDictValues *values = _PyDictOrValues_GetValues(dorv);
            PyObject *old_value = values->values[index];
            values->values[index] = value;
            if (old_value == NULL) {
                _PyDictValues_AddToInsertionOrder(values, index);
            }
            else {
                Py_DECREF(old_value);
            }
            Py_DECREF(owner);
        }

        inst(STORE_ATTR_WITH_HINT, (unused/1, type_version/2, hint/1, value, owner --)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *tp = Py_TYPE(owner);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, STORE_ATTR);
            assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(owner);
            DEOPT_IF(_PyDictOrValues_IsValues(dorv), STORE_ATTR);
            PyDictObject *dict = (PyDictObject *)_PyDictOrValues_GetDict(dorv);
            DEOPT_IF(dict == NULL, STORE_ATTR);
            assert(PyDict_CheckExact((PyObject *)dict));
            PyObject *name = GETITEM(names, oparg);
            DEOPT_IF(hint >= (size_t)dict->ma_keys->dk_nentries, STORE_ATTR);
            PyObject *old_value;
            uint64_t new_version;
            if (DK_IS_UNICODE(dict->ma_keys)) {
                PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, STORE_ATTR);
                old_value = ep->me_value;
                DEOPT_IF(old_value == NULL, STORE_ATTR);
                new_version = _PyDict_NotifyEvent(tstate->interp, PyDict_EVENT_MODIFIED, dict, name, value);
                ep->me_value = value;
            }
            else {
                PyDictKeyEntry *ep = DK_ENTRIES(dict->ma_keys) + hint;
                DEOPT_IF(ep->me_key != name, STORE_ATTR);
                old_value = ep->me_value;
                DEOPT_IF(old_value == NULL, STORE_ATTR);
                new_version = _PyDict_NotifyEvent(tstate->interp, PyDict_EVENT_MODIFIED, dict, name, value);
                ep->me_value = value;
            }
            Py_DECREF(old_value);
            STAT_INC(STORE_ATTR, hit);
            /* Ensure dict is GC tracked if it needs to be */
            if (!_PyObject_GC_IS_TRACKED(dict) && _PyObject_GC_MAY_BE_TRACKED(value)) {
                _PyObject_GC_TRACK(dict);
            }
            /* PEP 509 */
            dict->ma_version_tag = new_version;
            Py_DECREF(owner);
        }

        inst(STORE_ATTR_SLOT, (unused/1, type_version/2, index/1, value, owner --)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *tp = Py_TYPE(owner);
            assert(type_version != 0);
            DEOPT_IF(tp->tp_version_tag != type_version, STORE_ATTR);
            char *addr = (char *)owner + index;
            STAT_INC(STORE_ATTR, hit);
            PyObject *old_value = *(PyObject **)addr;
            *(PyObject **)addr = value;
            Py_XDECREF(old_value);
            Py_DECREF(owner);
        }

        inst(COMPARE_OP, (unused/1, left, right -- res)) {
            STAT_INC(COMPARE_OP, deferred);
            assert((oparg >> 4) <= Py_GE);
            res = PyObject_RichCompare(left, right, oparg>>4);
            Py_DECREF(left);
            Py_DECREF(right);
            ERROR_IF(res == NULL, error);
        }

        // No cache size here, since this is a family of super-instructions.
        family(compare_and_branch) = {
            COMPARE_AND_BRANCH,
            COMPARE_AND_BRANCH_FLOAT,
            COMPARE_AND_BRANCH_INT,
            COMPARE_AND_BRANCH_STR,
        };

        inst(COMPARE_AND_BRANCH, (unused/2, left, right -- )) {
            #if ENABLE_SPECIALIZATION
            _PyCompareOpCache *cache = (_PyCompareOpCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_CompareAndBranch(left, right, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(COMPARE_AND_BRANCH, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            assert((oparg >> 4) <= Py_GE);
            PyObject *cond = PyObject_RichCompare(left, right, oparg>>4);
            Py_DECREF(left);
            Py_DECREF(right);
            ERROR_IF(cond == NULL, error);
            assert(next_instr[1].op.code == POP_JUMP_IF_FALSE ||
                   next_instr[1].op.code == POP_JUMP_IF_TRUE);
            bool jump_on_true = next_instr[1].op.code == POP_JUMP_IF_TRUE;
            int offset = next_instr[1].op.arg;
            int err = PyObject_IsTrue(cond);
            Py_DECREF(cond);
            ERROR_IF(err < 0, error);
            if (jump_on_true == (err != 0)) {
                JUMPBY(offset);
            }
        }

        inst(COMPARE_AND_BRANCH_FLOAT, (unused/2, left, right -- )) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyFloat_CheckExact(left), COMPARE_AND_BRANCH);
            DEOPT_IF(!PyFloat_CheckExact(right), COMPARE_AND_BRANCH);
            STAT_INC(COMPARE_AND_BRANCH, hit);
            double dleft = PyFloat_AS_DOUBLE(left);
            double dright = PyFloat_AS_DOUBLE(right);
            // 1 if NaN, 2 if <, 4 if >, 8 if ==; this matches low four bits of the oparg
            int sign_ish = COMPARISON_BIT(dleft, dright);
            _Py_DECREF_SPECIALIZED(left, _PyFloat_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right, _PyFloat_ExactDealloc);
            if (sign_ish & oparg) {
                int offset = next_instr[1].op.arg;
                JUMPBY(offset);
            }
        }

        // Similar to COMPARE_AND_BRANCH_FLOAT
        inst(COMPARE_AND_BRANCH_INT, (unused/2, left, right -- )) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyLong_CheckExact(left), COMPARE_AND_BRANCH);
            DEOPT_IF(!PyLong_CheckExact(right), COMPARE_AND_BRANCH);
            DEOPT_IF((size_t)(Py_SIZE(left) + 1) > 2, COMPARE_AND_BRANCH);
            DEOPT_IF((size_t)(Py_SIZE(right) + 1) > 2, COMPARE_AND_BRANCH);
            STAT_INC(COMPARE_AND_BRANCH, hit);
            assert(Py_ABS(Py_SIZE(left)) <= 1 && Py_ABS(Py_SIZE(right)) <= 1);
            Py_ssize_t ileft = Py_SIZE(left) * ((PyLongObject *)left)->long_value.ob_digit[0];
            Py_ssize_t iright = Py_SIZE(right) * ((PyLongObject *)right)->long_value.ob_digit[0];
            // 2 if <, 4 if >, 8 if ==; this matches the low 4 bits of the oparg
            int sign_ish = COMPARISON_BIT(ileft, iright);
            _Py_DECREF_SPECIALIZED(left, (destructor)PyObject_Free);
            _Py_DECREF_SPECIALIZED(right, (destructor)PyObject_Free);
            if (sign_ish & oparg) {
                int offset = next_instr[1].op.arg;
                JUMPBY(offset);
            }
        }

        // Similar to COMPARE_AND_BRANCH_FLOAT, but for ==, != only
        inst(COMPARE_AND_BRANCH_STR, (unused/2, left, right -- )) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(!PyUnicode_CheckExact(left), COMPARE_AND_BRANCH);
            DEOPT_IF(!PyUnicode_CheckExact(right), COMPARE_AND_BRANCH);
            STAT_INC(COMPARE_AND_BRANCH, hit);
            int res = _PyUnicode_Equal(left, right);
            assert((oparg >>4) == Py_EQ || (oparg >>4) == Py_NE);
            _Py_DECREF_SPECIALIZED(left, _PyUnicode_ExactDealloc);
            _Py_DECREF_SPECIALIZED(right, _PyUnicode_ExactDealloc);
            assert(res == 0 || res == 1);
            assert((oparg & 0xf) == COMPARISON_NOT_EQUALS || (oparg & 0xf) == COMPARISON_EQUALS);
            assert(COMPARISON_NOT_EQUALS + 1 == COMPARISON_EQUALS);
            if ((res + COMPARISON_NOT_EQUALS) & oparg) {
                int offset = next_instr[1].op.arg;
                JUMPBY(offset);
            }
        }

        inst(IS_OP, (left, right -- b)) {
            int res = Py_Is(left, right) ^ oparg;
            DECREF_INPUTS();
            b = Py_NewRef(res ? Py_True : Py_False);
        }

        inst(CONTAINS_OP, (left, right -- b)) {
            int res = PySequence_Contains(right, left);
            DECREF_INPUTS();
            ERROR_IF(res < 0, error);
            b = Py_NewRef((res^oparg) ? Py_True : Py_False);
        }

        inst(CHECK_EG_MATCH, (exc_value, match_type -- rest, match)) {
            if (check_except_star_type_valid(tstate, match_type) < 0) {
                DECREF_INPUTS();
                ERROR_IF(true, error);
            }

            match = NULL;
            rest = NULL;
            int res = exception_group_match(exc_value, match_type,
                                            &match, &rest);
            DECREF_INPUTS();
            ERROR_IF(res < 0, error);

            assert((match == NULL) == (rest == NULL));
            ERROR_IF(match == NULL, error);

            if (!Py_IsNone(match)) {
                PyErr_SetExcInfo(NULL, Py_NewRef(match), NULL);
            }
        }

        inst(CHECK_EXC_MATCH, (left, right -- left, b)) {
            assert(PyExceptionInstance_Check(left));
            if (check_except_type_valid(tstate, right) < 0) {
                 DECREF_INPUTS();
                 ERROR_IF(true, error);
            }

            int res = PyErr_GivenExceptionMatches(left, right);
            DECREF_INPUTS();
            b = Py_NewRef(res ? Py_True : Py_False);
        }

         inst(IMPORT_NAME, (level, fromlist -- res)) {
            PyObject *name = GETITEM(names, oparg);
            res = import_name(tstate, frame, name, fromlist, level);
            DECREF_INPUTS();
            ERROR_IF(res == NULL, error);
        }

        inst(IMPORT_FROM, (from -- from, res)) {
            PyObject *name = GETITEM(names, oparg);
            res = import_from(tstate, from, name);
            ERROR_IF(res == NULL, error);
        }

        inst(JUMP_FORWARD, (--)) {
            JUMPBY(oparg);
        }

        inst(JUMP_BACKWARD, (--)) {
            assert(oparg < INSTR_OFFSET());
            JUMPBY(-oparg);
            CHECK_EVAL_BREAKER();
        }

        inst(POP_JUMP_IF_FALSE, (cond -- )) {
            if (Py_IsTrue(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
            }
            else if (Py_IsFalse(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
                JUMPBY(oparg);
            }
            else {
                int err = PyObject_IsTrue(cond);
                Py_DECREF(cond);
                if (err == 0) {
                    JUMPBY(oparg);
                }
                else {
                    ERROR_IF(err < 0, error);
                }
            }
        }

        inst(POP_JUMP_IF_TRUE, (cond -- )) {
            if (Py_IsFalse(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
            }
            else if (Py_IsTrue(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
                JUMPBY(oparg);
            }
            else {
                int err = PyObject_IsTrue(cond);
                Py_DECREF(cond);
                if (err > 0) {
                    JUMPBY(oparg);
                }
                else {
                    ERROR_IF(err < 0, error);
                }
            }
        }

        inst(POP_JUMP_IF_NOT_NONE, (value -- )) {
            if (!Py_IsNone(value)) {
                Py_DECREF(value);
                JUMPBY(oparg);
            }
            else {
                _Py_DECREF_NO_DEALLOC(value);
            }
        }

        inst(POP_JUMP_IF_NONE, (value -- )) {
            if (Py_IsNone(value)) {
                _Py_DECREF_NO_DEALLOC(value);
                JUMPBY(oparg);
            }
            else {
                Py_DECREF(value);
            }
        }

        inst(JUMP_IF_FALSE_OR_POP, (cond -- cond if (jump))) {
            bool jump = false;
            int err;
            if (Py_IsTrue(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
            }
            else if (Py_IsFalse(cond)) {
                JUMPBY(oparg);
                jump = true;
            }
            else {
                err = PyObject_IsTrue(cond);
                if (err > 0) {
                    Py_DECREF(cond);
                }
                else if (err == 0) {
                    JUMPBY(oparg);
                    jump = true;
                }
                else {
                    goto error;
                }
            }
        }

        inst(JUMP_IF_TRUE_OR_POP, (cond -- cond if (jump))) {
            bool jump = false;
            int err;
            if (Py_IsFalse(cond)) {
                _Py_DECREF_NO_DEALLOC(cond);
            }
            else if (Py_IsTrue(cond)) {
                JUMPBY(oparg);
                jump = true;
            }
            else {
                err = PyObject_IsTrue(cond);
                if (err > 0) {
                    JUMPBY(oparg);
                    jump = true;
                }
                else if (err == 0) {
                    Py_DECREF(cond);
                }
                else {
                    goto error;
                }
            }
        }

        inst(JUMP_BACKWARD_NO_INTERRUPT, (--)) {
            /* This bytecode is used in the `yield from` or `await` loop.
             * If there is an interrupt, we want it handled in the innermost
             * generator or coroutine, so we deliberately do not check it here.
             * (see bpo-30039).
             */
            JUMPBY(-oparg);
        }

        inst(GET_LEN, (obj -- obj, len_o)) {
            // PUSH(len(TOS))
            Py_ssize_t len_i = PyObject_Length(obj);
            ERROR_IF(len_i < 0, error);
            len_o = PyLong_FromSsize_t(len_i);
            ERROR_IF(len_o == NULL, error);
        }

        inst(MATCH_CLASS, (subject, type, names -- attrs)) {
            // Pop TOS and TOS1. Set TOS to a tuple of attributes on success, or
            // None on failure.
            assert(PyTuple_CheckExact(names));
            attrs = match_class(tstate, subject, type, oparg, names);
            DECREF_INPUTS();
            if (attrs) {
                assert(PyTuple_CheckExact(attrs));  // Success!
            }
            else {
                ERROR_IF(_PyErr_Occurred(tstate), error);  // Error!
                attrs = Py_NewRef(Py_None);  // Failure!
            }
        }

        inst(MATCH_MAPPING, (subject -- subject, res)) {
            int match = Py_TYPE(subject)->tp_flags & Py_TPFLAGS_MAPPING;
            res = Py_NewRef(match ? Py_True : Py_False);
            PREDICT(POP_JUMP_IF_FALSE);
        }

        inst(MATCH_SEQUENCE, (subject -- subject, res)) {
            int match = Py_TYPE(subject)->tp_flags & Py_TPFLAGS_SEQUENCE;
            res = Py_NewRef(match ? Py_True : Py_False);
            PREDICT(POP_JUMP_IF_FALSE);
        }

        inst(MATCH_KEYS, (subject, keys -- subject, keys, values_or_none)) {
            // On successful match, PUSH(values). Otherwise, PUSH(None).
            values_or_none = match_keys(tstate, subject, keys);
            ERROR_IF(values_or_none == NULL, error);
        }

        inst(GET_ITER, (iterable -- iter)) {
            /* before: [obj]; after [getiter(obj)] */
            iter = PyObject_GetIter(iterable);
            DECREF_INPUTS();
            ERROR_IF(iter == NULL, error);
        }

        inst(GET_YIELD_FROM_ITER, (iterable -- iter)) {
            /* before: [obj]; after [getiter(obj)] */
            if (PyCoro_CheckExact(iterable)) {
                /* `iterable` is a coroutine */
                if (!(frame->f_code->co_flags & (CO_COROUTINE | CO_ITERABLE_COROUTINE))) {
                    /* and it is used in a 'yield from' expression of a
                       regular generator. */
                    _PyErr_SetString(tstate, PyExc_TypeError,
                                     "cannot 'yield from' a coroutine object "
                                     "in a non-coroutine generator");
                    goto error;
                }
                iter = iterable;
            }
            else if (PyGen_CheckExact(iterable)) {
                iter = iterable;
            }
            else {
                /* `iterable` is not a generator. */
                iter = PyObject_GetIter(iterable);
                if (iter == NULL) {
                    goto error;
                }
                Py_DECREF(iterable);
            }
            PREDICT(LOAD_CONST);
        }

        // Most members of this family are "secretly" super-instructions.
        // When the loop is exhausted, they jump, and the jump target is
        // always END_FOR, which pops two values off the stack.
        // This is optimized by skipping that instruction and combining
        // its effect (popping 'iter' instead of pushing 'next'.)

        family(for_iter, INLINE_CACHE_ENTRIES_FOR_ITER) = {
            FOR_ITER,
            FOR_ITER_LIST,
            FOR_ITER_TUPLE,
            FOR_ITER_RANGE,
            FOR_ITER_GEN,
        };

        inst(FOR_ITER, (unused/1, iter -- iter, next)) {
            #if ENABLE_SPECIALIZATION
            _PyForIterCache *cache = (_PyForIterCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_ForIter(iter, next_instr, oparg);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(FOR_ITER, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            /* before: [iter]; after: [iter, iter()] *or* [] (and jump over END_FOR.) */
            next = (*Py_TYPE(iter)->tp_iternext)(iter);
            if (next == NULL) {
                if (_PyErr_Occurred(tstate)) {
                    if (!_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
                        goto error;
                    }
                    else if (tstate->c_tracefunc != NULL) {
                        call_exc_trace(tstate->c_tracefunc, tstate->c_traceobj, tstate, frame);
                    }
                    _PyErr_Clear(tstate);
                }
                /* iterator ended normally */
                assert(next_instr[INLINE_CACHE_ENTRIES_FOR_ITER + oparg].op.code == END_FOR);
                Py_DECREF(iter);
                STACK_SHRINK(1);
                /* Jump forward oparg, then skip following END_FOR instruction */
                JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1);
                DISPATCH();
            }
            // Common case: no jump, leave it to the code generator
        }

        inst(FOR_ITER_LIST, (unused/1, iter -- iter, next)) {
            assert(cframe.use_tracing == 0);
            DEOPT_IF(Py_TYPE(iter) != &PyListIter_Type, FOR_ITER);
            _PyListIterObject *it = (_PyListIterObject *)iter;
            STAT_INC(FOR_ITER, hit);
            PyListObject *seq = it->it_seq;
            if (seq) {
                if (it->it_index < PyList_GET_SIZE(seq)) {
                    next = Py_NewRef(PyList_GET_ITEM(seq, it->it_index++));
                    goto end_for_iter_list;  // End of this instruction
                }
                it->it_seq = NULL;
                Py_DECREF(seq);
            }
            Py_DECREF(iter);
            STACK_SHRINK(1);
            /* Jump forward oparg, then skip following END_FOR instruction */
            JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1);
            DISPATCH();
        end_for_iter_list:
            // Common case: no jump, leave it to the code generator
        }

        inst(FOR_ITER_TUPLE, (unused/1, iter -- iter, next)) {
            assert(cframe.use_tracing == 0);
            _PyTupleIterObject *it = (_PyTupleIterObject *)iter;
            DEOPT_IF(Py_TYPE(it) != &PyTupleIter_Type, FOR_ITER);
            STAT_INC(FOR_ITER, hit);
            PyTupleObject *seq = it->it_seq;
            if (seq) {
                if (it->it_index < PyTuple_GET_SIZE(seq)) {
                    next = Py_NewRef(PyTuple_GET_ITEM(seq, it->it_index++));
                    goto end_for_iter_tuple;  // End of this instruction
                }
                it->it_seq = NULL;
                Py_DECREF(seq);
            }
            Py_DECREF(iter);
            STACK_SHRINK(1);
            /* Jump forward oparg, then skip following END_FOR instruction */
            JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1);
            DISPATCH();
        end_for_iter_tuple:
            // Common case: no jump, leave it to the code generator
        }

        inst(FOR_ITER_RANGE, (unused/1, iter -- iter, next)) {
            assert(cframe.use_tracing == 0);
            _PyRangeIterObject *r = (_PyRangeIterObject *)iter;
            DEOPT_IF(Py_TYPE(r) != &PyRangeIter_Type, FOR_ITER);
            STAT_INC(FOR_ITER, hit);
            if (r->len <= 0) {
                STACK_SHRINK(1);
                Py_DECREF(r);
                // Jump over END_FOR instruction.
                JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg + 1);
                DISPATCH();
            }
            long value = r->start;
            r->start = value + r->step;
            r->len--;
            next = PyLong_FromLong(value);
            if (next == NULL) {
                goto error;
            }
        }

        inst(FOR_ITER_GEN, (unused/1, iter -- iter, unused)) {
            assert(cframe.use_tracing == 0);
            PyGenObject *gen = (PyGenObject *)iter;
            DEOPT_IF(Py_TYPE(gen) != &PyGen_Type, FOR_ITER);
            DEOPT_IF(gen->gi_frame_state >= FRAME_EXECUTING, FOR_ITER);
            STAT_INC(FOR_ITER, hit);
            _PyInterpreterFrame *gen_frame = (_PyInterpreterFrame *)gen->gi_iframe;
            frame->yield_offset = oparg;
            _PyFrame_StackPush(gen_frame, Py_NewRef(Py_None));
            gen->gi_frame_state = FRAME_EXECUTING;
            gen->gi_exc_state.previous_item = tstate->exc_info;
            tstate->exc_info = &gen->gi_exc_state;
            JUMPBY(INLINE_CACHE_ENTRIES_FOR_ITER + oparg);
            assert(next_instr->op.code == END_FOR);
            DISPATCH_INLINED(gen_frame);
        }

        inst(BEFORE_ASYNC_WITH, (mgr -- exit, res)) {
            PyObject *enter = _PyObject_LookupSpecial(mgr, &_Py_ID(__aenter__));
            if (enter == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "asynchronous context manager protocol",
                                  Py_TYPE(mgr)->tp_name);
                }
                goto error;
            }
            exit = _PyObject_LookupSpecial(mgr, &_Py_ID(__aexit__));
            if (exit == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "asynchronous context manager protocol "
                                  "(missed __aexit__ method)",
                                  Py_TYPE(mgr)->tp_name);
                }
                Py_DECREF(enter);
                goto error;
            }
            DECREF_INPUTS();
            res = _PyObject_CallNoArgs(enter);
            Py_DECREF(enter);
            if (res == NULL) {
                Py_DECREF(exit);
                ERROR_IF(true, error);
            }
            PREDICT(GET_AWAITABLE);
        }

        inst(BEFORE_WITH, (mgr -- exit, res)) {
            /* pop the context manager, push its __exit__ and the
             * value returned from calling its __enter__
             */
            PyObject *enter = _PyObject_LookupSpecial(mgr, &_Py_ID(__enter__));
            if (enter == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "context manager protocol",
                                  Py_TYPE(mgr)->tp_name);
                }
                goto error;
            }
            exit = _PyObject_LookupSpecial(mgr, &_Py_ID(__exit__));
            if (exit == NULL) {
                if (!_PyErr_Occurred(tstate)) {
                    _PyErr_Format(tstate, PyExc_TypeError,
                                  "'%.200s' object does not support the "
                                  "context manager protocol "
                                  "(missed __exit__ method)",
                                  Py_TYPE(mgr)->tp_name);
                }
                Py_DECREF(enter);
                goto error;
            }
            DECREF_INPUTS();
            res = _PyObject_CallNoArgs(enter);
            Py_DECREF(enter);
            if (res == NULL) {
                Py_DECREF(exit);
                ERROR_IF(true, error);
            }
        }

        inst(WITH_EXCEPT_START, (exit_func, lasti, unused, val -- exit_func, lasti, unused, val, res)) {
            /* At the top of the stack are 4 values:
               - val: TOP = exc_info()
               - unused: SECOND = previous exception
               - lasti: THIRD = lasti of exception in exc_info()
               - exit_func: FOURTH = the context.__exit__ bound method
               We call FOURTH(type(TOP), TOP, GetTraceback(TOP)).
               Then we push the __exit__ return value.
            */
            PyObject *exc, *tb;

            assert(val && PyExceptionInstance_Check(val));
            exc = PyExceptionInstance_Class(val);
            tb = PyException_GetTraceback(val);
            Py_XDECREF(tb);
            assert(PyLong_Check(lasti));
            (void)lasti; // Shut up compiler warning if asserts are off
            PyObject *stack[4] = {NULL, exc, val, tb};
            res = PyObject_Vectorcall(exit_func, stack + 1,
                    3 | PY_VECTORCALL_ARGUMENTS_OFFSET, NULL);
            ERROR_IF(res == NULL, error);
        }

        inst(PUSH_EXC_INFO, (new_exc -- prev_exc, new_exc)) {
            _PyErr_StackItem *exc_info = tstate->exc_info;
            if (exc_info->exc_value != NULL) {
                prev_exc = exc_info->exc_value;
            }
            else {
                prev_exc = Py_NewRef(Py_None);
            }
            assert(PyExceptionInstance_Check(new_exc));
            exc_info->exc_value = Py_NewRef(new_exc);
        }

        inst(LOAD_ATTR_METHOD_WITH_VALUES, (unused/1, type_version/2, keys_version/2, descr/4, self -- res2 if (oparg & 1), res)) {
            /* Cached method object */
            assert(cframe.use_tracing == 0);
            PyTypeObject *self_cls = Py_TYPE(self);
            assert(type_version != 0);
            DEOPT_IF(self_cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(self_cls->tp_flags & Py_TPFLAGS_MANAGED_DICT);
            PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(self);
            DEOPT_IF(!_PyDictOrValues_IsValues(dorv), LOAD_ATTR);
            PyHeapTypeObject *self_heap_type = (PyHeapTypeObject *)self_cls;
            DEOPT_IF(self_heap_type->ht_cached_keys->dk_version !=
                     keys_version, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            res2 = Py_NewRef(descr);
            assert(_PyType_HasFeature(Py_TYPE(res2), Py_TPFLAGS_METHOD_DESCRIPTOR));
            res = self;
            assert(oparg & 1);
        }

        inst(LOAD_ATTR_METHOD_NO_DICT, (unused/1, type_version/2, unused/2, descr/4, self -- res2 if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *self_cls = Py_TYPE(self);
            DEOPT_IF(self_cls->tp_version_tag != type_version, LOAD_ATTR);
            assert(self_cls->tp_dictoffset == 0);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            res2 = Py_NewRef(descr);
            res = self;
            assert(oparg & 1);
        }

        inst(LOAD_ATTR_METHOD_LAZY_DICT, (unused/1, type_version/2, unused/2, descr/4, self -- res2 if (oparg & 1), res)) {
            assert(cframe.use_tracing == 0);
            PyTypeObject *self_cls = Py_TYPE(self);
            DEOPT_IF(self_cls->tp_version_tag != type_version, LOAD_ATTR);
            Py_ssize_t dictoffset = self_cls->tp_dictoffset;
            assert(dictoffset > 0);
            PyObject *dict = *(PyObject **)((char *)self + dictoffset);
            /* This object has a __dict__, just not yet created */
            DEOPT_IF(dict != NULL, LOAD_ATTR);
            STAT_INC(LOAD_ATTR, hit);
            assert(descr != NULL);
            assert(_PyType_HasFeature(Py_TYPE(descr), Py_TPFLAGS_METHOD_DESCRIPTOR));
            res2 = Py_NewRef(descr);
            res = self;
            assert(oparg & 1);
        }

        inst(KW_NAMES, (--)) {
            assert(kwnames == NULL);
            assert(oparg < PyTuple_GET_SIZE(consts));
            kwnames = GETITEM(consts, oparg);
        }

        // Cache layout: counter/1, func_version/2, min_args/1
        // Neither CALL_INTRINSIC_1/2 nor CALL_FUNCTION_EX are members!
        family(call, INLINE_CACHE_ENTRIES_CALL) = {
            CALL,
            CALL_BOUND_METHOD_EXACT_ARGS,
            CALL_PY_EXACT_ARGS,
            CALL_PY_WITH_DEFAULTS,
            CALL_NO_KW_TYPE_1,
            CALL_NO_KW_STR_1,
            CALL_NO_KW_TUPLE_1,
            CALL_BUILTIN_CLASS,
            CALL_NO_KW_BUILTIN_O,
            CALL_NO_KW_BUILTIN_FAST,
            CALL_BUILTIN_FAST_WITH_KEYWORDS,
            CALL_NO_KW_LEN,
            CALL_NO_KW_ISINSTANCE,
            CALL_NO_KW_LIST_APPEND,
            CALL_NO_KW_METHOD_DESCRIPTOR_O,
            CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS,
            CALL_NO_KW_METHOD_DESCRIPTOR_NOARGS,
            CALL_NO_KW_METHOD_DESCRIPTOR_FAST,
        };

        // On entry, the stack is either
        //   [NULL, callable, arg1, arg2, ...]
        // or
        //   [method, self, arg1, arg2, ...]
        // (Some args may be keywords, see KW_NAMES, which sets 'kwnames'.)
        // On exit, the stack is [result].
        // When calling Python, inline the call using DISPATCH_INLINED().
        inst(CALL, (unused/1, unused/2, unused/1, method, callable, args[oparg] -- res)) {
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                callable = method;
                args--;
                total_args++;
            }
            #if ENABLE_SPECIALIZATION
            _PyCallCache *cache = (_PyCallCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_Call(callable, next_instr, total_args, kwnames);
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(CALL, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            if (!is_meth && Py_TYPE(callable) == &PyMethod_Type) {
                is_meth = 1;  // For consistenct; it's dead, though
                args--;
                total_args++;
                PyObject *self = ((PyMethodObject *)callable)->im_self;
                args[0] = Py_NewRef(self);
                method = ((PyMethodObject *)callable)->im_func;
                args[-1] = Py_NewRef(method);
                Py_DECREF(callable);
                callable = method;
            }
            int positional_args = total_args - KWNAMES_LEN();
            // Check if the call can be inlined or not
            if (Py_TYPE(callable) == &PyFunction_Type &&
                tstate->interp->eval_frame == NULL &&
                ((PyFunctionObject *)callable)->vectorcall == _PyFunction_Vectorcall)
            {
                int code_flags = ((PyCodeObject*)PyFunction_GET_CODE(callable))->co_flags;
                PyObject *locals = code_flags & CO_OPTIMIZED ? NULL : Py_NewRef(PyFunction_GET_GLOBALS(callable));
                _PyInterpreterFrame *new_frame = _PyEvalFramePushAndInit(
                    tstate, (PyFunctionObject *)callable, locals,
                    args, positional_args, kwnames
                );
                kwnames = NULL;
                // Manipulate stack directly since we leave using DISPATCH_INLINED().
                STACK_SHRINK(oparg + 2);
                // The frame has stolen all the arguments from the stack,
                // so there is no need to clean them up.
                if (new_frame == NULL) {
                    goto error;
                }
                JUMPBY(INLINE_CACHE_ENTRIES_CALL);
                DISPATCH_INLINED(new_frame);
            }
            /* Callable is not a normal Python function */
            if (cframe.use_tracing) {
                res = trace_call_function(
                    tstate, callable, args,
                    positional_args, kwnames);
            }
            else {
                res = PyObject_Vectorcall(
                    callable, args,
                    positional_args | PY_VECTORCALL_ARGUMENTS_OFFSET,
                    kwnames);
            }
            kwnames = NULL;
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            Py_DECREF(callable);
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(args[i]);
            }
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        // Start out with [NULL, bound_method, arg1, arg2, ...]
        // Transform to [callable, self, arg1, arg2, ...]
        // Then fall through to CALL_PY_EXACT_ARGS
        inst(CALL_BOUND_METHOD_EXACT_ARGS, (unused/1, unused/2, unused/1, method, callable, unused[oparg] -- unused)) {
            DEOPT_IF(method != NULL, CALL);
            DEOPT_IF(Py_TYPE(callable) != &PyMethod_Type, CALL);
            STAT_INC(CALL, hit);
            PyObject *self = ((PyMethodObject *)callable)->im_self;
            PEEK(oparg + 1) = Py_NewRef(self);  // callable
            PyObject *meth = ((PyMethodObject *)callable)->im_func;
            PEEK(oparg + 2) = Py_NewRef(meth);  // method
            Py_DECREF(callable);
            GO_TO_INSTRUCTION(CALL_PY_EXACT_ARGS);
        }

        inst(CALL_PY_EXACT_ARGS, (unused/1, func_version/2, unused/1, method, callable, args[oparg] -- unused)) {
            assert(kwnames == NULL);
            DEOPT_IF(tstate->interp->eval_frame, CALL);
            int is_meth = method != NULL;
            int argcount = oparg;
            if (is_meth) {
                callable = method;
                args--;
                argcount++;
            }
            DEOPT_IF(!PyFunction_Check(callable), CALL);
            PyFunctionObject *func = (PyFunctionObject *)callable;
            DEOPT_IF(func->func_version != func_version, CALL);
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            DEOPT_IF(code->co_argcount != argcount, CALL);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), CALL);
            STAT_INC(CALL, hit);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, func, argcount);
            for (int i = 0; i < argcount; i++) {
                new_frame->localsplus[i] = args[i];
            }
            // Manipulate stack directly since we leave using DISPATCH_INLINED().
            STACK_SHRINK(oparg + 2);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            DISPATCH_INLINED(new_frame);
        }

        inst(CALL_PY_WITH_DEFAULTS, (unused/1, func_version/2, min_args/1, method, callable, args[oparg] -- unused)) {
            assert(kwnames == NULL);
            DEOPT_IF(tstate->interp->eval_frame, CALL);
            int is_meth = method != NULL;
            int argcount = oparg;
            if (is_meth) {
                callable = method;
                args--;
                argcount++;
            }
            DEOPT_IF(!PyFunction_Check(callable), CALL);
            PyFunctionObject *func = (PyFunctionObject *)callable;
            DEOPT_IF(func->func_version != func_version, CALL);
            PyCodeObject *code = (PyCodeObject *)func->func_code;
            DEOPT_IF(argcount > code->co_argcount, CALL);
            DEOPT_IF(argcount < min_args, CALL);
            DEOPT_IF(!_PyThreadState_HasStackSpace(tstate, code->co_framesize), CALL);
            STAT_INC(CALL, hit);
            _PyInterpreterFrame *new_frame = _PyFrame_PushUnchecked(tstate, func, code->co_argcount);
            for (int i = 0; i < argcount; i++) {
                new_frame->localsplus[i] = args[i];
            }
            for (int i = argcount; i < code->co_argcount; i++) {
                PyObject *def = PyTuple_GET_ITEM(func->func_defaults, i - min_args);
                new_frame->localsplus[i] = Py_NewRef(def);
            }
            // Manipulate stack and cache directly since we leave using DISPATCH_INLINED().
            STACK_SHRINK(oparg + 2);
            JUMPBY(INLINE_CACHE_ENTRIES_CALL);
            DISPATCH_INLINED(new_frame);
        }

        inst(CALL_NO_KW_TYPE_1, (unused/1, unused/2, unused/1, null, callable, args[oparg] -- res)) {
            assert(kwnames == NULL);
            assert(cframe.use_tracing == 0);
            assert(oparg == 1);
            DEOPT_IF(null != NULL, CALL);
            PyObject *obj = args[0];
            DEOPT_IF(callable != (PyObject *)&PyType_Type, CALL);
            STAT_INC(CALL, hit);
            res = Py_NewRef(Py_TYPE(obj));
            Py_DECREF(obj);
            Py_DECREF(&PyType_Type);  // I.e., callable
        }

        inst(CALL_NO_KW_STR_1, (unused/1, unused/2, unused/1, null, callable, args[oparg] -- res)) {
            assert(kwnames == NULL);
            assert(cframe.use_tracing == 0);
            assert(oparg == 1);
            DEOPT_IF(null != NULL, CALL);
            DEOPT_IF(callable != (PyObject *)&PyUnicode_Type, CALL);
            STAT_INC(CALL, hit);
            PyObject *arg = args[0];
            res = PyObject_Str(arg);
            Py_DECREF(arg);
            Py_DECREF(&PyUnicode_Type);  // I.e., callable
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_NO_KW_TUPLE_1, (unused/1, unused/2, unused/1, null, callable, args[oparg] -- res)) {
            assert(kwnames == NULL);
            assert(oparg == 1);
            DEOPT_IF(null != NULL, CALL);
            DEOPT_IF(callable != (PyObject *)&PyTuple_Type, CALL);
            STAT_INC(CALL, hit);
            PyObject *arg = args[0];
            res = PySequence_Tuple(arg);
            Py_DECREF(arg);
            Py_DECREF(&PyTuple_Type);  // I.e., tuple
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_BUILTIN_CLASS, (unused/1, unused/2, unused/1, method, callable, args[oparg] -- res)) {
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                callable = method;
                args--;
                total_args++;
            }
            int kwnames_len = KWNAMES_LEN();
            DEOPT_IF(!PyType_Check(callable), CALL);
            PyTypeObject *tp = (PyTypeObject *)callable;
            DEOPT_IF(tp->tp_vectorcall == NULL, CALL);
            STAT_INC(CALL, hit);
            res = tp->tp_vectorcall((PyObject *)tp, args,
                                    total_args - kwnames_len, kwnames);
            kwnames = NULL;
            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(args[i]);
            }
            Py_DECREF(tp);
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_NO_KW_BUILTIN_O, (unused/1, unused/2, unused/1, method, callable, args[oparg] -- res)) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_O functions */
            assert(kwnames == NULL);
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                callable = method;
                args--;
                total_args++;
            }
            DEOPT_IF(total_args != 1, CALL);
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) != METH_O, CALL);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable);
            // This is slower but CPython promises to check all non-vectorcall
            // function calls.
            if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
                goto error;
            }
            PyObject *arg = args[0];
            res = _PyCFunction_TrampolineCall(cfunc, PyCFunction_GET_SELF(callable), arg);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            Py_DECREF(arg);
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_NO_KW_BUILTIN_FAST, (unused/1, unused/2, unused/1, method, callable, args[oparg] -- res)) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_FASTCALL functions, without keywords */
            assert(kwnames == NULL);
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                callable = method;
                args--;
                total_args++;
            }
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) != METH_FASTCALL, CALL);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = PyCFunction_GET_FUNCTION(callable);
            /* res = func(self, args, nargs) */
            res = ((_PyCFunctionFast)(void(*)(void))cfunc)(
                PyCFunction_GET_SELF(callable),
                args,
                total_args);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(args[i]);
            }
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
                /* Not deopting because this doesn't mean our optimization was
                   wrong. `res` can be NULL for valid reasons. Eg. getattr(x,
                   'invalid'). In those cases an exception is set, so we must
                   handle it.
                */
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_BUILTIN_FAST_WITH_KEYWORDS, (unused/1, unused/2, unused/1, method, callable, args[oparg] -- res)) {
            assert(cframe.use_tracing == 0);
            /* Builtin METH_FASTCALL | METH_KEYWORDS functions */
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                callable = method;
                args--;
                total_args++;
            }
            DEOPT_IF(!PyCFunction_CheckExact(callable), CALL);
            DEOPT_IF(PyCFunction_GET_FLAGS(callable) !=
                (METH_FASTCALL | METH_KEYWORDS), CALL);
            STAT_INC(CALL, hit);
            /* res = func(self, args, nargs, kwnames) */
            _PyCFunctionFastWithKeywords cfunc =
                (_PyCFunctionFastWithKeywords)(void(*)(void))
                PyCFunction_GET_FUNCTION(callable);
            res = cfunc(
                PyCFunction_GET_SELF(callable),
                args,
                total_args - KWNAMES_LEN(),
                kwnames
            );
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            kwnames = NULL;

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(args[i]);
            }
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_NO_KW_LEN, (unused/1, unused/2, unused/1, method, callable, args[oparg] -- res)) {
            assert(cframe.use_tracing == 0);
            assert(kwnames == NULL);
            /* len(o) */
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                callable = method;
                args--;
                total_args++;
            }
            DEOPT_IF(total_args != 1, CALL);
            PyInterpreterState *interp = _PyInterpreterState_GET();
            DEOPT_IF(callable != interp->callable_cache.len, CALL);
            STAT_INC(CALL, hit);
            PyObject *arg = args[0];
            Py_ssize_t len_i = PyObject_Length(arg);
            if (len_i < 0) {
                goto error;
            }
            res = PyLong_FromSsize_t(len_i);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            Py_DECREF(callable);
            Py_DECREF(arg);
            ERROR_IF(res == NULL, error);
        }

        inst(CALL_NO_KW_ISINSTANCE, (unused/1, unused/2, unused/1, method, callable, args[oparg] -- res)) {
            assert(cframe.use_tracing == 0);
            assert(kwnames == NULL);
            /* isinstance(o, o2) */
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                callable = method;
                args--;
                total_args++;
            }
            DEOPT_IF(total_args != 2, CALL);
            PyInterpreterState *interp = _PyInterpreterState_GET();
            DEOPT_IF(callable != interp->callable_cache.isinstance, CALL);
            STAT_INC(CALL, hit);
            PyObject *cls = args[1];
            PyObject *inst = args[0];
            int retval = PyObject_IsInstance(inst, cls);
            if (retval < 0) {
                goto error;
            }
            res = PyBool_FromLong(retval);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));

            Py_DECREF(inst);
            Py_DECREF(cls);
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
        }

        // This is secretly a super-instruction
        inst(CALL_NO_KW_LIST_APPEND, (unused/1, unused/2, unused/1, method, self, args[oparg] -- unused)) {
            assert(cframe.use_tracing == 0);
            assert(kwnames == NULL);
            assert(oparg == 1);
            assert(method != NULL);
            PyInterpreterState *interp = _PyInterpreterState_GET();
            DEOPT_IF(method != interp->callable_cache.list_append, CALL);
            DEOPT_IF(!PyList_Check(self), CALL);
            STAT_INC(CALL, hit);
            if (_PyList_AppendTakeRef((PyListObject *)self, args[0]) < 0) {
                goto pop_1_error;  // Since arg is DECREF'ed already
            }
            Py_DECREF(self);
            Py_DECREF(method);
            STACK_SHRINK(3);
            // CALL + POP_TOP
            JUMPBY(INLINE_CACHE_ENTRIES_CALL + 1);
            assert(next_instr[-1].op.code == POP_TOP);
            DISPATCH();
        }

        inst(CALL_NO_KW_METHOD_DESCRIPTOR_O, (unused/1, unused/2, unused/1, method, unused, args[oparg] -- res)) {
            assert(kwnames == NULL);
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                args--;
                total_args++;
            }
            PyMethodDescrObject *callable =
                (PyMethodDescrObject *)PEEK(total_args + 1);
            DEOPT_IF(total_args != 2, CALL);
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            DEOPT_IF(meth->ml_flags != METH_O, CALL);
            PyObject *arg = args[1];
            PyObject *self = args[0];
            DEOPT_IF(!Py_IS_TYPE(self, callable->d_common.d_type), CALL);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = meth->ml_meth;
            // This is slower but CPython promises to check all non-vectorcall
            // function calls.
            if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
                goto error;
            }
            res = _PyCFunction_TrampolineCall(cfunc, self, arg);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            Py_DECREF(self);
            Py_DECREF(arg);
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS, (unused/1, unused/2, unused/1, method, unused, args[oparg] -- res)) {
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                args--;
                total_args++;
            }
            PyMethodDescrObject *callable =
                (PyMethodDescrObject *)PEEK(total_args + 1);
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            DEOPT_IF(meth->ml_flags != (METH_FASTCALL|METH_KEYWORDS), CALL);
            PyTypeObject *d_type = callable->d_common.d_type;
            PyObject *self = args[0];
            DEOPT_IF(!Py_IS_TYPE(self, d_type), CALL);
            STAT_INC(CALL, hit);
            int nargs = total_args - 1;
            _PyCFunctionFastWithKeywords cfunc =
                (_PyCFunctionFastWithKeywords)(void(*)(void))meth->ml_meth;
            res = cfunc(self, args + 1, nargs - KWNAMES_LEN(), kwnames);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            kwnames = NULL;

            /* Free the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(args[i]);
            }
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_NO_KW_METHOD_DESCRIPTOR_NOARGS, (unused/1, unused/2, unused/1, method, unused, args[oparg] -- res)) {
            assert(kwnames == NULL);
            assert(oparg == 0 || oparg == 1);
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                args--;
                total_args++;
            }
            DEOPT_IF(total_args != 1, CALL);
            PyMethodDescrObject *callable = (PyMethodDescrObject *)SECOND();
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            PyObject *self = args[0];
            DEOPT_IF(!Py_IS_TYPE(self, callable->d_common.d_type), CALL);
            DEOPT_IF(meth->ml_flags != METH_NOARGS, CALL);
            STAT_INC(CALL, hit);
            PyCFunction cfunc = meth->ml_meth;
            // This is slower but CPython promises to check all non-vectorcall
            // function calls.
            if (_Py_EnterRecursiveCallTstate(tstate, " while calling a Python object")) {
                goto error;
            }
            res = _PyCFunction_TrampolineCall(cfunc, self, NULL);
            _Py_LeaveRecursiveCallTstate(tstate);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            Py_DECREF(self);
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_NO_KW_METHOD_DESCRIPTOR_FAST, (unused/1, unused/2, unused/1, method, unused, args[oparg] -- res)) {
            assert(kwnames == NULL);
            int is_meth = method != NULL;
            int total_args = oparg;
            if (is_meth) {
                args--;
                total_args++;
            }
            PyMethodDescrObject *callable =
                (PyMethodDescrObject *)PEEK(total_args + 1);
            /* Builtin METH_FASTCALL methods, without keywords */
            DEOPT_IF(!Py_IS_TYPE(callable, &PyMethodDescr_Type), CALL);
            PyMethodDef *meth = callable->d_method;
            DEOPT_IF(meth->ml_flags != METH_FASTCALL, CALL);
            PyObject *self = args[0];
            DEOPT_IF(!Py_IS_TYPE(self, callable->d_common.d_type), CALL);
            STAT_INC(CALL, hit);
            _PyCFunctionFast cfunc =
                (_PyCFunctionFast)(void(*)(void))meth->ml_meth;
            int nargs = total_args - 1;
            res = cfunc(self, args + 1, nargs);
            assert((res != NULL) ^ (_PyErr_Occurred(tstate) != NULL));
            /* Clear the stack of the arguments. */
            for (int i = 0; i < total_args; i++) {
                Py_DECREF(args[i]);
            }
            Py_DECREF(callable);
            ERROR_IF(res == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(CALL_FUNCTION_EX, (unused, func, callargs, kwargs if (oparg & 1) -- result)) {
            if (oparg & 1) {
                // DICT_MERGE is called before this opcode if there are kwargs.
                // It converts all dict subtypes in kwargs into regular dicts.
                assert(PyDict_CheckExact(kwargs));
            }
            if (!PyTuple_CheckExact(callargs)) {
                if (check_args_iterable(tstate, func, callargs) < 0) {
                    goto error;
                }
                PyObject *tuple = PySequence_Tuple(callargs);
                if (tuple == NULL) {
                    goto error;
                }
                Py_SETREF(callargs, tuple);
            }
            assert(PyTuple_CheckExact(callargs));

            result = do_call_core(tstate, func, callargs, kwargs, cframe.use_tracing);
            Py_DECREF(func);
            Py_DECREF(callargs);
            Py_XDECREF(kwargs);

            assert(PEEK(3 + (oparg & 1)) == NULL);
            ERROR_IF(result == NULL, error);
            CHECK_EVAL_BREAKER();
        }

        inst(MAKE_FUNCTION, (defaults    if (oparg & 0x01),
                             kwdefaults  if (oparg & 0x02),
                             annotations if (oparg & 0x04),
                             closure     if (oparg & 0x08),
                             codeobj -- func)) {

            PyFunctionObject *func_obj = (PyFunctionObject *)
                PyFunction_New(codeobj, GLOBALS());

            Py_DECREF(codeobj);
            if (func_obj == NULL) {
                goto error;
            }

            if (oparg & 0x08) {
                assert(PyTuple_CheckExact(closure));
                func_obj->func_closure = closure;
            }
            if (oparg & 0x04) {
                assert(PyTuple_CheckExact(annotations));
                func_obj->func_annotations = annotations;
            }
            if (oparg & 0x02) {
                assert(PyDict_CheckExact(kwdefaults));
                func_obj->func_kwdefaults = kwdefaults;
            }
            if (oparg & 0x01) {
                assert(PyTuple_CheckExact(defaults));
                func_obj->func_defaults = defaults;
            }

            func_obj->func_version = ((PyCodeObject *)codeobj)->co_version;
            func = (PyObject *)func_obj;
        }

        inst(RETURN_GENERATOR, (--)) {
            assert(PyFunction_Check(frame->f_funcobj));
            PyFunctionObject *func = (PyFunctionObject *)frame->f_funcobj;
            PyGenObject *gen = (PyGenObject *)_Py_MakeCoro(func);
            if (gen == NULL) {
                goto error;
            }
            assert(EMPTY());
            _PyFrame_SetStackPointer(frame, stack_pointer);
            _PyInterpreterFrame *gen_frame = (_PyInterpreterFrame *)gen->gi_iframe;
            _PyFrame_Copy(frame, gen_frame);
            assert(frame->frame_obj == NULL);
            gen->gi_frame_state = FRAME_CREATED;
            gen_frame->owner = FRAME_OWNED_BY_GENERATOR;
            _Py_LeaveRecursiveCallPy(tstate);
            assert(frame != &entry_frame);
            _PyInterpreterFrame *prev = frame->previous;
            _PyThreadState_PopFrame(tstate, frame);
            frame = cframe.current_frame = prev;
            _PyFrame_StackPush(frame, (PyObject *)gen);
            goto resume_frame;
        }

        inst(BUILD_SLICE, (start, stop, step if (oparg == 3) -- slice)) {
            slice = PySlice_New(start, stop, step);
            Py_DECREF(start);
            Py_DECREF(stop);
            Py_XDECREF(step);
            ERROR_IF(slice == NULL, error);
        }

        inst(FORMAT_VALUE, (value, fmt_spec if ((oparg & FVS_MASK) == FVS_HAVE_SPEC) -- result)) {
            /* Handles f-string value formatting. */
            PyObject *(*conv_fn)(PyObject *);
            int which_conversion = oparg & FVC_MASK;

            /* See if any conversion is specified. */
            switch (which_conversion) {
            case FVC_NONE:  conv_fn = NULL;           break;
            case FVC_STR:   conv_fn = PyObject_Str;   break;
            case FVC_REPR:  conv_fn = PyObject_Repr;  break;
            case FVC_ASCII: conv_fn = PyObject_ASCII; break;
            default:
                _PyErr_Format(tstate, PyExc_SystemError,
                              "unexpected conversion flag %d",
                              which_conversion);
                goto error;
            }

            /* If there's a conversion function, call it and replace
               value with that result. Otherwise, just use value,
               without conversion. */
            if (conv_fn != NULL) {
                result = conv_fn(value);
                Py_DECREF(value);
                if (result == NULL) {
                    Py_XDECREF(fmt_spec);
                    ERROR_IF(true, error);
                }
                value = result;
            }

            /* If value is a unicode object, and there's no fmt_spec,
               then we know the result of format(value) is value
               itself. In that case, skip calling format(). I plan to
               move this optimization in to PyObject_Format()
               itself. */
            if (PyUnicode_CheckExact(value) && fmt_spec == NULL) {
                /* Do nothing, just transfer ownership to result. */
                result = value;
            } else {
                /* Actually call format(). */
                result = PyObject_Format(value, fmt_spec);
                Py_DECREF(value);
                Py_XDECREF(fmt_spec);
                ERROR_IF(result == NULL, error);
            }
        }

        inst(COPY, (bottom, unused[oparg-1] -- bottom, unused[oparg-1], top)) {
            assert(oparg > 0);
            top = Py_NewRef(bottom);
        }

        inst(BINARY_OP, (unused/1, lhs, rhs -- res)) {
            #if ENABLE_SPECIALIZATION
            _PyBinaryOpCache *cache = (_PyBinaryOpCache *)next_instr;
            if (ADAPTIVE_COUNTER_IS_ZERO(cache->counter)) {
                assert(cframe.use_tracing == 0);
                next_instr--;
                _Py_Specialize_BinaryOp(lhs, rhs, next_instr, oparg, &GETLOCAL(0));
                DISPATCH_SAME_OPARG();
            }
            STAT_INC(BINARY_OP, deferred);
            DECREMENT_ADAPTIVE_COUNTER(cache->counter);
            #endif  /* ENABLE_SPECIALIZATION */
            assert(0 <= oparg);
            assert((unsigned)oparg < Py_ARRAY_LENGTH(binary_ops));
            assert(binary_ops[oparg]);
            res = binary_ops[oparg](lhs, rhs);
            Py_DECREF(lhs);
            Py_DECREF(rhs);
            ERROR_IF(res == NULL, error);
        }

        inst(SWAP, (bottom, unused[oparg-2], top --
                    top, unused[oparg-2], bottom)) {
            assert(oparg >= 2);
        }

        inst(EXTENDED_ARG, (--)) {
            assert(oparg);
            assert(cframe.use_tracing == 0);
            opcode = next_instr->op.code;
            oparg = oparg << 8 | next_instr->op.arg;
            PRE_DISPATCH_GOTO();
            DISPATCH_GOTO();
        }

        inst(CACHE, (--)) {
            Py_UNREACHABLE();
        }


// END BYTECODES //

    }
 dispatch_opcode:
 error:
 exception_unwind:
 exit_unwind:
 handle_eval_breaker:
 resume_frame:
 resume_with_error:
 start_frame:
 unbound_local_error:
    ;
}

// Future families go below this point //

family(store_fast) = { STORE_FAST, STORE_FAST__LOAD_FAST, STORE_FAST__STORE_FAST };
