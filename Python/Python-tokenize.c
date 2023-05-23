#include "Python.h"
#include "errcode.h"
#include "../Parser/tokenizer.h"
#include "../Parser/pegen.h"      // _PyPegen_byte_offset_to_character_offset()
#include "../Parser/pegen.h"      // _PyPegen_byte_offset_to_character_offset()

static struct PyModuleDef _tokenizemodule;

typedef struct {
    PyTypeObject *TokenizerIter;
} tokenize_state;

static tokenize_state *
get_tokenize_state(PyObject *module) {
    return (tokenize_state *)PyModule_GetState(module);
}

#define _tokenize_get_state_by_type(type) \
    get_tokenize_state(PyType_GetModuleByDef(type, &_tokenizemodule))

#include "pycore_runtime.h"
#include "clinic/Python-tokenize.c.h"

/*[clinic input]
module _tokenizer
class _tokenizer.tokenizeriter "tokenizeriterobject *" "_tokenize_get_state_by_type(type)->TokenizerIter"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=96d98ee2fef7a8bc]*/

typedef struct
{
    PyObject_HEAD struct tok_state *tok;
} tokenizeriterobject;

/*[clinic input]
@classmethod
_tokenizer.tokenizeriter.__new__ as tokenizeriter_new

    source: str
    *
    extra_tokens: bool
[clinic start generated code]*/

static PyObject *
tokenizeriter_new_impl(PyTypeObject *type, const char *source,
                       int extra_tokens)
/*[clinic end generated code: output=f6f9d8b4beec8106 input=90dc5b6a5df180c2]*/
{
    tokenizeriterobject *self = (tokenizeriterobject *)type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    PyObject *filename = PyUnicode_FromString("<string>");
    if (filename == NULL) {
        return NULL;
    }
    self->tok = _PyTokenizer_FromUTF8(source, 1);
    if (self->tok == NULL) {
        Py_DECREF(filename);
        return NULL;
    }
    self->tok->filename = filename;
    if (extra_tokens) {
        self->tok->tok_extra_tokens = 1;
    }
    return (PyObject *)self;
}

static int
_tokenizer_error(struct tok_state *tok)
{
    if (PyErr_Occurred()) {
        return -1;
    }

    const char *msg = NULL;
    PyObject* errtype = PyExc_SyntaxError;
    switch (tok->done) {
        case E_TOKEN:
            msg = "invalid token";
            break;
        case E_EOF:
            if (tok->level) {
                    PyErr_Format(PyExc_SyntaxError,
                                 "parenthesis '%c' was never closed",
                                tok->parenstack[tok->level-1]);
            } else {
                PyErr_SetString(PyExc_SyntaxError, "unexpected EOF while parsing");
            }
            return -1;
        case E_DEDENT:
            msg = "unindent does not match any outer indentation level";
            errtype = PyExc_IndentationError;
            break;
        case E_INTR:
            if (!PyErr_Occurred()) {
                PyErr_SetNone(PyExc_KeyboardInterrupt);
            }
            return -1;
        case E_NOMEM:
            PyErr_NoMemory();
            return -1;
        case E_TABSPACE:
            errtype = PyExc_TabError;
            msg = "inconsistent use of tabs and spaces in indentation";
            break;
        case E_TOODEEP:
            errtype = PyExc_IndentationError;
            msg = "too many levels of indentation";
            break;
        case E_LINECONT: {
            msg = "unexpected character after line continuation character";
            break;
        }
        default:
            msg = "unknown tokenization error";
    }

    PyObject* errstr = NULL;
    PyObject* error_line = NULL;
    PyObject* tmp = NULL;
    PyObject* value = NULL;
    int result = 0;

    Py_ssize_t size = tok->inp - tok->buf;
    error_line = PyUnicode_DecodeUTF8(tok->buf, size, "replace");
    if (!error_line) {
        result = -1;
        goto exit;
    }

    Py_ssize_t offset = _PyPegen_byte_offset_to_character_offset(error_line, tok->inp - tok->buf);
    if (offset == -1) {
        result = -1;
        goto exit;
    }
    tmp = Py_BuildValue("(OnnOOO)", tok->filename, tok->lineno, offset, error_line, Py_None, Py_None);
    if (!tmp) {
        result = -1;
        goto exit;
    }

    errstr = PyUnicode_FromString(msg);
    if (!errstr) {
        result = -1;
        goto exit;
    }

    value = PyTuple_Pack(2, errstr, tmp);
    if (!value) {
        result = -1;
        goto exit;
    }

    PyErr_SetObject(errtype, value);

exit:
    Py_XDECREF(errstr);
    Py_XDECREF(error_line);
    Py_XDECREF(tmp);
    Py_XDECREF(value);
    return result;
}

static PyObject *
tokenizeriter_next(tokenizeriterobject *it)
{
    PyObject* result = NULL;
    struct token token;
    _PyToken_Init(&token);

    int type = _PyTokenizer_Get(it->tok, &token);
    if (type == ERRORTOKEN) {
        if(!PyErr_Occurred()) {
            _tokenizer_error(it->tok);
            assert(PyErr_Occurred());
        }
        goto exit;
    }
    if (type == ERRORTOKEN || type == ENDMARKER) {
        PyErr_SetString(PyExc_StopIteration, "EOF");
        goto exit;
    }
    PyObject *str = NULL;
    if (token.start == NULL || token.end == NULL) {
        str = PyUnicode_FromString("");
    }
    else {
        str = PyUnicode_FromStringAndSize(token.start, token.end - token.start);
    }
    if (str == NULL) {
        goto exit;
    }

    Py_ssize_t size = it->tok->inp - it->tok->buf;
    PyObject *line = PyUnicode_DecodeUTF8(it->tok->buf, size, "replace");
    if (line == NULL) {
        Py_DECREF(str);
        goto exit;
    }
    const char *line_start = ISSTRINGLIT(type) ? it->tok->multi_line_start : it->tok->line_start;
    Py_ssize_t lineno = ISSTRINGLIT(type) ? it->tok->first_lineno : it->tok->lineno;
    Py_ssize_t end_lineno = it->tok->lineno;
    Py_ssize_t col_offset = -1;
    Py_ssize_t end_col_offset = -1;
    if (token.start != NULL && token.start >= line_start) {
        col_offset = _PyPegen_byte_offset_to_character_offset(line, token.start - line_start);
    }
    if (token.end != NULL && token.end >= it->tok->line_start) {
        end_col_offset = _PyPegen_byte_offset_to_character_offset(line, token.end - it->tok->line_start);
    }

    if (it->tok->tok_extra_tokens) {
        // Necessary adjustments to match the original Python tokenize
        // implementation
        if (type > DEDENT && type < OP) {
            type = OP;
        }
        else if (type == ASYNC || type == AWAIT) {
            type = NAME;
        }
        else if (type == NEWLINE) {
            str = PyUnicode_FromString("\n");
            end_col_offset++;
        }
    }

    result = Py_BuildValue("(iN(nn)(nn)N)", type, str, lineno, col_offset, end_lineno, end_col_offset, line);
exit:
    _PyToken_Free(&token);
    return result;
}

static void
tokenizeriter_dealloc(tokenizeriterobject *it)
{
    PyTypeObject *tp = Py_TYPE(it);
    _PyTokenizer_Free(it->tok);
    tp->tp_free(it);
    Py_DECREF(tp);
}

static PyType_Slot tokenizeriter_slots[] = {
    {Py_tp_new, tokenizeriter_new},
    {Py_tp_dealloc, tokenizeriter_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, tokenizeriter_next},
    {0, NULL},
};

static PyType_Spec tokenizeriter_spec = {
    .name = "_tokenize.TokenizerIter",
    .basicsize = sizeof(tokenizeriterobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = tokenizeriter_slots,
};

static int
tokenizemodule_exec(PyObject *m)
{
    tokenize_state *state = get_tokenize_state(m);
    if (state == NULL) {
        return -1;
    }

    state->TokenizerIter = (PyTypeObject *)PyType_FromModuleAndSpec(m, &tokenizeriter_spec, NULL);
    if (state->TokenizerIter == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, state->TokenizerIter) < 0) {
        return -1;
    }

    return 0;
}

static PyMethodDef tokenize_methods[] = {
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static PyModuleDef_Slot tokenizemodule_slots[] = {
    {Py_mod_exec, tokenizemodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static int
tokenizemodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    tokenize_state *state = get_tokenize_state(m);
    Py_VISIT(state->TokenizerIter);
    return 0;
}

static int
tokenizemodule_clear(PyObject *m)
{
    tokenize_state *state = get_tokenize_state(m);
    Py_CLEAR(state->TokenizerIter);
    return 0;
}

static void
tokenizemodule_free(void *m)
{
    tokenizemodule_clear((PyObject *)m);
}

static struct PyModuleDef _tokenizemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_tokenize",
    .m_size = sizeof(tokenize_state),
    .m_slots = tokenizemodule_slots,
    .m_methods = tokenize_methods,
    .m_traverse = tokenizemodule_traverse,
    .m_clear = tokenizemodule_clear,
    .m_free = tokenizemodule_free,
};

PyMODINIT_FUNC
PyInit__tokenize(void)
{
    return PyModuleDef_Init(&_tokenizemodule);
}
