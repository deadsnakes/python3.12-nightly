/* Function object interface */

#ifndef Py_LIMITED_API
#ifndef Py_FUNCOBJECT_H
#define Py_FUNCOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


#define COMMON_FIELDS(PREFIX) \
    PyObject *PREFIX ## globals; \
    PyObject *PREFIX ## builtins; \
    PyObject *PREFIX ## name; \
    PyObject *PREFIX ## qualname; \
    PyObject *PREFIX ## code;        /* A code object, the __code__ attribute */ \
    PyObject *PREFIX ## defaults;    /* NULL or a tuple */ \
    PyObject *PREFIX ## kwdefaults;  /* NULL or a dict */ \
    PyObject *PREFIX ## closure;     /* NULL or a tuple of cell objects */

typedef struct {
    COMMON_FIELDS(fc_)
} PyFrameConstructor;

/* Function objects and code objects should not be confused with each other:
 *
 * Function objects are created by the execution of the 'def' statement.
 * They reference a code object in their __code__ attribute, which is a
 * purely syntactic object, i.e. nothing more than a compiled version of some
 * source code lines.  There is one code object per source code "fragment",
 * but each code object can be referenced by zero or many function objects
 * depending only on how many times the 'def' statement in the source was
 * executed so far.
 */

typedef struct {
    PyObject_HEAD
    COMMON_FIELDS(func_)
    PyObject *func_doc;         /* The __doc__ attribute, can be anything */
    PyObject *func_dict;        /* The __dict__ attribute, a dict or NULL */
    PyObject *func_weakreflist; /* List of weak references */
    PyObject *func_module;      /* The __module__ attribute, can be anything */
    PyObject *func_annotations; /* Annotations, a dict or NULL */
    vectorcallfunc vectorcall;
    /* Version number for use by specializer.
     * Can set to non-zero when we want to specialize.
     * Will be set to zero if any of these change:
     *     defaults
     *     kwdefaults (only if the object changes, not the contents of the dict)
     *     code
     *     annotations
     *     vectorcall function pointer */
    uint32_t func_version;

    /* Invariant:
     *     func_closure contains the bindings for func_code->co_freevars, so
     *     PyTuple_Size(func_closure) == PyCode_GetNumFree(func_code)
     *     (func_closure may be NULL if PyCode_GetNumFree(func_code) == 0).
     */
} PyFunctionObject;

PyAPI_DATA(PyTypeObject) PyFunction_Type;

#define PyFunction_Check(op) Py_IS_TYPE((op), &PyFunction_Type)

PyAPI_FUNC(PyObject *) PyFunction_New(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_NewWithQualName(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetCode(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetGlobals(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetModule(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetDefaults(PyObject *, PyObject *);
PyAPI_FUNC(void) PyFunction_SetVectorcall(PyFunctionObject *, vectorcallfunc);
PyAPI_FUNC(PyObject *) PyFunction_GetKwDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetKwDefaults(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetClosure(PyObject *);
PyAPI_FUNC(int) PyFunction_SetClosure(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetAnnotations(PyObject *);
PyAPI_FUNC(int) PyFunction_SetAnnotations(PyObject *, PyObject *);

PyAPI_FUNC(PyObject *) _PyFunction_Vectorcall(
    PyObject *func,
    PyObject *const *stack,
    size_t nargsf,
    PyObject *kwnames);

#define _PyFunction_CAST(func) \
    (assert(PyFunction_Check(func)), _Py_CAST(PyFunctionObject*, func))

/* Static inline functions for direct access to these values.
   Type checks are *not* done, so use with care. */
static inline PyObject* PyFunction_GET_CODE(PyObject *func) {
    return _PyFunction_CAST(func)->func_code;
}
#define PyFunction_GET_CODE(func) PyFunction_GET_CODE(_PyObject_CAST(func))

static inline PyObject* PyFunction_GET_GLOBALS(PyObject *func) {
    return _PyFunction_CAST(func)->func_globals;
}
#define PyFunction_GET_GLOBALS(func) PyFunction_GET_GLOBALS(_PyObject_CAST(func))

static inline PyObject* PyFunction_GET_MODULE(PyObject *func) {
    return _PyFunction_CAST(func)->func_module;
}
#define PyFunction_GET_MODULE(func) PyFunction_GET_MODULE(_PyObject_CAST(func))

static inline PyObject* PyFunction_GET_DEFAULTS(PyObject *func) {
    return _PyFunction_CAST(func)->func_defaults;
}
#define PyFunction_GET_DEFAULTS(func) PyFunction_GET_DEFAULTS(_PyObject_CAST(func))

static inline PyObject* PyFunction_GET_KW_DEFAULTS(PyObject *func) {
    return _PyFunction_CAST(func)->func_kwdefaults;
}
#define PyFunction_GET_KW_DEFAULTS(func) PyFunction_GET_KW_DEFAULTS(_PyObject_CAST(func))

static inline PyObject* PyFunction_GET_CLOSURE(PyObject *func) {
    return _PyFunction_CAST(func)->func_closure;
}
#define PyFunction_GET_CLOSURE(func) PyFunction_GET_CLOSURE(_PyObject_CAST(func))

static inline PyObject* PyFunction_GET_ANNOTATIONS(PyObject *func) {
    return _PyFunction_CAST(func)->func_annotations;
}
#define PyFunction_GET_ANNOTATIONS(func) PyFunction_GET_ANNOTATIONS(_PyObject_CAST(func))

/* The classmethod and staticmethod types lives here, too */
PyAPI_DATA(PyTypeObject) PyClassMethod_Type;
PyAPI_DATA(PyTypeObject) PyStaticMethod_Type;

PyAPI_FUNC(PyObject *) PyClassMethod_New(PyObject *);
PyAPI_FUNC(PyObject *) PyStaticMethod_New(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_FUNCOBJECT_H */
#endif /* Py_LIMITED_API */
