import doctest
import textwrap
import unittest


doctests = """
########### Tests borrowed from or inspired by test_genexps.py ############

Test simple loop with conditional

    >>> sum([i*i for i in range(100) if i&1 == 1])
    166650

Test simple nesting

    >>> [(i,j) for i in range(3) for j in range(4)]
    [(0, 0), (0, 1), (0, 2), (0, 3), (1, 0), (1, 1), (1, 2), (1, 3), (2, 0), (2, 1), (2, 2), (2, 3)]

Test nesting with the inner expression dependent on the outer

    >>> [(i,j) for i in range(4) for j in range(i)]
    [(1, 0), (2, 0), (2, 1), (3, 0), (3, 1), (3, 2)]

Test the idiom for temporary variable assignment in comprehensions.

    >>> [j*j for i in range(4) for j in [i+1]]
    [1, 4, 9, 16]
    >>> [j*k for i in range(4) for j in [i+1] for k in [j+1]]
    [2, 6, 12, 20]
    >>> [j*k for i in range(4) for j, k in [(i+1, i+2)]]
    [2, 6, 12, 20]

Not assignment

    >>> [i*i for i in [*range(4)]]
    [0, 1, 4, 9]
    >>> [i*i for i in (*range(4),)]
    [0, 1, 4, 9]

Make sure the induction variable is not exposed

    >>> i = 20
    >>> sum([i*i for i in range(100)])
    328350

    >>> i
    20

Verify that syntax error's are raised for listcomps used as lvalues

    >>> [y for y in (1,2)] = 10          # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...

    >>> [y for y in (1,2)] += 10         # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...


########### Tests borrowed from or inspired by test_generators.py ############

Make a nested list comprehension that acts like range()

    >>> def frange(n):
    ...     return [i for i in range(n)]
    >>> frange(10)
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

Same again, only as a lambda expression instead of a function definition

    >>> lrange = lambda n:  [i for i in range(n)]
    >>> lrange(10)
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

Generators can call other generators:

    >>> def grange(n):
    ...     for x in [i for i in range(n)]:
    ...         yield x
    >>> list(grange(5))
    [0, 1, 2, 3, 4]


Make sure that None is a valid return value

    >>> [None for i in range(10)]
    [None, None, None, None, None, None, None, None, None, None]

"""


class ListComprehensionTest(unittest.TestCase):
    def _check_in_scopes(self, code, outputs=None, ns=None, scopes=None, raises=()):
        code = textwrap.dedent(code)
        scopes = scopes or ["module", "class", "function"]
        for scope in scopes:
            with self.subTest(scope=scope):
                if scope == "class":
                    newcode = textwrap.dedent("""
                        class _C:
                            {code}
                    """).format(code=textwrap.indent(code, "    "))
                    def get_output(moddict, name):
                        return getattr(moddict["_C"], name)
                elif scope == "function":
                    newcode = textwrap.dedent("""
                        def _f():
                            {code}
                            return locals()
                        _out = _f()
                    """).format(code=textwrap.indent(code, "    "))
                    def get_output(moddict, name):
                        return moddict["_out"][name]
                else:
                    newcode = code
                    def get_output(moddict, name):
                        return moddict[name]
                ns = ns or {}
                try:
                    exec(newcode, ns)
                except raises as e:
                    # We care about e.g. NameError vs UnboundLocalError
                    self.assertIs(type(e), raises)
                else:
                    for k, v in (outputs or {}).items():
                        self.assertEqual(get_output(ns, k), v)

    def test_lambdas_with_iteration_var_as_default(self):
        code = """
            items = [(lambda i=i: i) for i in range(5)]
            y = [x() for x in items]
        """
        outputs = {"y": [0, 1, 2, 3, 4]}
        self._check_in_scopes(code, outputs)

    def test_lambdas_with_free_var(self):
        code = """
            items = [(lambda: i) for i in range(5)]
            y = [x() for x in items]
        """
        outputs = {"y": [4, 4, 4, 4, 4]}
        self._check_in_scopes(code, outputs)

    def test_class_scope_free_var_with_class_cell(self):
        class C:
            def method(self):
                super()
                return __class__
            items = [(lambda: i) for i in range(5)]
            y = [x() for x in items]

        self.assertEqual(C.y, [4, 4, 4, 4, 4])
        self.assertIs(C().method(), C)

    def test_inner_cell_shadows_outer(self):
        code = """
            items = [(lambda: i) for i in range(5)]
            i = 20
            y = [x() for x in items]
        """
        outputs = {"y": [4, 4, 4, 4, 4], "i": 20}
        self._check_in_scopes(code, outputs)

    def test_inner_cell_shadows_outer_no_store(self):
        code = """
            def f(x):
                return [lambda: x for x in range(x)], x
            fns, x = f(2)
            y = [fn() for fn in fns]
        """
        outputs = {"y": [1, 1], "x": 2}
        self._check_in_scopes(code, outputs)

    def test_closure_can_jump_over_comp_scope(self):
        code = """
            items = [(lambda: y) for i in range(5)]
            y = 2
            z = [x() for x in items]
        """
        outputs = {"z": [2, 2, 2, 2, 2]}
        self._check_in_scopes(code, outputs)

    def test_inner_cell_shadows_outer_redefined(self):
        code = """
            y = 10
            items = [(lambda: y) for y in range(5)]
            x = y
            y = 20
            out = [z() for z in items]
        """
        outputs = {"x": 10, "out": [4, 4, 4, 4, 4]}
        self._check_in_scopes(code, outputs)

    def test_shadows_outer_cell(self):
        code = """
            def inner():
                return g
            [g for g in range(5)]
            x = inner()
        """
        outputs = {"x": -1}
        self._check_in_scopes(code, outputs, ns={"g": -1})

    def test_assignment_expression(self):
        code = """
            x = -1
            items = [(x:=y) for y in range(3)]
        """
        outputs = {"x": 2}
        # assignment expression in comprehension is disallowed in class scope
        self._check_in_scopes(code, outputs, scopes=["module", "function"])

    def test_free_var_in_comp_child(self):
        code = """
            lst = range(3)
            funcs = [lambda: x for x in lst]
            inc = [x + 1 for x in lst]
            [x for x in inc]
            x = funcs[0]()
        """
        outputs = {"x": 2}
        self._check_in_scopes(code, outputs)

    def test_shadow_with_free_and_local(self):
        code = """
            lst = range(3)
            x = -1
            funcs = [lambda: x for x in lst]
            items = [x + 1 for x in lst]
        """
        outputs = {"x": -1}
        self._check_in_scopes(code, outputs)

    def test_shadow_comp_iterable_name(self):
        code = """
            x = [1]
            y = [x for x in x]
        """
        outputs = {"x": [1]}
        self._check_in_scopes(code, outputs)

    def test_nested_free(self):
        code = """
            x = 1
            def g():
                [x for x in range(3)]
                return x
            g()
        """
        outputs = {"x": 1}
        self._check_in_scopes(code, outputs)

    def test_introspecting_frame_locals(self):
        code = """
            import sys
            [i for i in range(2)]
            i = 20
            sys._getframe().f_locals
        """
        outputs = {"i": 20}
        self._check_in_scopes(code, outputs)

    def test_nested(self):
        code = """
            l = [2, 3]
            y = [[x ** 2 for x in range(x)] for x in l]
        """
        outputs = {"y": [[0, 1], [0, 1, 4]]}
        self._check_in_scopes(code, outputs)

    def test_nested_2(self):
        code = """
            l = [1, 2, 3]
            x = 3
            y = [x for [x ** x for x in range(x)][x - 1] in l]
        """
        outputs = {"y": [3, 3, 3]}
        self._check_in_scopes(code, outputs)

    def test_nested_3(self):
        code = """
            l = [(1, 2), (3, 4), (5, 6)]
            y = [x for (x, [x ** x for x in range(x)][x - 1]) in l]
        """
        outputs = {"y": [1, 3, 5]}
        self._check_in_scopes(code, outputs)

    def test_nameerror(self):
        code = """
            [x for x in [1]]
            x
        """

        self._check_in_scopes(code, raises=NameError)

    def test_dunder_name(self):
        code = """
            y = [__x for __x in [1]]
        """
        outputs = {"y": [1]}
        self._check_in_scopes(code, outputs)

    def test_unbound_local_after_comprehension(self):
        def f():
            if False:
                x = 0
            [x for x in [1]]
            return x

        with self.assertRaises(UnboundLocalError):
            f()

    def test_unbound_local_inside_comprehension(self):
        def f():
            l = [None]
            return [1 for (l[0], l) in [[1, 2]]]

        with self.assertRaises(UnboundLocalError):
            f()


__test__ = {'doctests' : doctests}

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
