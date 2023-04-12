#ifndef Py_INTERNAL_COMPILE_H
#define Py_INTERNAL_COMPILE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

struct _arena;   // Type defined in pycore_pyarena.h
struct _mod;     // Type defined in pycore_ast.h

// Export the symbol for test_peg_generator (built as a library)
PyAPI_FUNC(PyCodeObject*) _PyAST_Compile(
    struct _mod *mod,
    PyObject *filename,
    PyCompilerFlags *flags,
    int optimize,
    struct _arena *arena);


typedef struct {
    int optimize;
    int ff_features;

    int recursion_depth;            /* current recursion depth */
    int recursion_limit;            /* recursion limit */
} _PyASTOptimizeState;

extern int _PyAST_Optimize(
    struct _mod *,
    struct _arena *arena,
    _PyASTOptimizeState *state);


typedef struct {
    int i_opcode;
    int i_oparg;
    _PyCompilerSrcLocation i_loc;
} _PyCompilerInstruction;

typedef struct {
    _PyCompilerInstruction *s_instrs;
    int s_allocated;
    int s_used;

    int *s_labelmap;       /* label id --> instr offset */
    int s_labelmap_size;
    int s_next_free_label; /* next free label id */
} _PyCompile_InstructionSequence;

typedef struct {
    PyObject *u_name;
    PyObject *u_qualname;  /* dot-separated qualified name (lazy) */

    /* The following fields are dicts that map objects to
       the index of them in co_XXX.      The index is used as
       the argument for opcodes that refer to those collections.
    */
    PyObject *u_consts;    /* all constants */
    PyObject *u_names;     /* all names */
    PyObject *u_varnames;  /* local variables */
    PyObject *u_cellvars;  /* cell variables */
    PyObject *u_freevars;  /* free variables */

    Py_ssize_t u_argcount;        /* number of arguments for block */
    Py_ssize_t u_posonlyargcount;        /* number of positional only arguments for block */
    Py_ssize_t u_kwonlyargcount; /* number of keyword only arguments for block */

    int u_firstlineno; /* the first lineno of the block */
} _PyCompile_CodeUnitMetadata;


/* Utility for a number of growing arrays used in the compiler */
int _PyCompile_EnsureArrayLargeEnough(
        int idx,
        void **array,
        int *alloc,
        int default_alloc,
        size_t item_size);

int _PyCompile_ConstCacheMergeOne(PyObject *const_cache, PyObject **obj);

/* Access compiler internals for unit testing */

PyAPI_FUNC(PyObject*) _PyCompile_CodeGen(
        PyObject *ast,
        PyObject *filename,
        PyCompilerFlags *flags,
        int optimize);

PyAPI_FUNC(PyObject*) _PyCompile_OptimizeCfg(
        PyObject *instructions,
        PyObject *consts);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_COMPILE_H */
