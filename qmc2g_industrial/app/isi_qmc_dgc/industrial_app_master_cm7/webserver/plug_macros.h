/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#ifndef CIT_HTTPD_PLUG_MACROS_H
#define CIT_HTTPD_PLUG_MACROS_H

/**
 *   macro definitions that are used to generate the plug callback types from the definition "macro-list" in plug.h
 */

#ifndef STATIC_ASSERT
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)
#ifdef __CDT_PARSER__
#define _Static_assert(...)
#endif
#else
#define STATIC_ASSERT(...)
#endif
#endif

/* concatenate two strings */
#define _PLUG_CAT(a, ...)     a##__VA_ARGS__

/* concatenate three strings */
#define _PLUG_CAT3(a, b, ...) a##b##__VA_ARGS__

/* get no arguments */
#define _PLUG_DROP(...)

/* get all arguments */
#define _PLUG_PASS(...) __VA_ARGS__

/* get first macro argument */
#define _PLUG_FIRST(a, ...) a

/* get second macro argument */
#define _PLUG_SECOND(a, b, ...) b

/* magic value to be detectable by _PLUG_IS_PROBE */
#define _PLUG_PROBE() ~, 1

/* detect if _PLUG_PROBE was passed */
#define _PLUG_IS_PROBE(...) _PLUG_SECOND(__VA_ARGS__, 0)

/* if 0 is passed return 1, else return 0 (from _PLUG_IS_PROBE) */
#define _PLUG_NOT(b) _PLUG_IS_PROBE(_PLUG_CAT(_PLUG_NOT_, b))
#define _PLUG_NOT_0  _PLUG_PROBE()

/* 0 -> 0 , any -> 1 */
#define _PLUG_BOOL(b) _PLUG_NOT(_PLUG_NOT(b))

/* check if the first argument is non empty and return a 1. 0 otherwise */
#define _PLUG_HAS_ARGS(...)              _PLUG_BOOL(_PLUG_FIRST(_PLUG_HAS_ARGS_END_OF_ARGS_ __VA_ARGS__)(0))
#define _PLUG_HAS_ARGS_END_OF_ARGS_(...) _PLUG_BOOL(_PLUG_FIRST(__VA_ARGS__))

/* if 'void'  is passed return 1, else return 0 */
#define _PLUG_IS_VOID(v)   _PLUG_IS_PROBE(_PLUG_CAT(_PLUG_IS_VOID_, v))
#define _PLUG_IS_VOID_void _PLUG_PROBE()

/* if 'plug_state_p'  is passed return 1, else return 0 */
#define _PLUG_IS_PLUG_STATE_P(v)           _PLUG_BOOL(_PLUG_IS_PROBE(_PLUG_CAT(_PLUG_IS_PLUG_STATE_P_, v)))
#define _PLUG_IS_PLUG_STATE_P_plug_state_p _PLUG_PROBE()

/* if 'plug_state_p'  is passed replace it with 'abstract_plug_state_t' */
#define _PLUG_ABSTRACT_PLUG_STATE_P(v)           _PLUG_CAT(_PLUG_ABSTRACT_PLUG_STATE_P_, v)
#define _PLUG_ABSTRACT_PLUG_STATE_P_plug_state_p void *

/* if  _PLUG_IF(condition)(then) */
#define _PLUG_IF(cond) _PLUG_IF_(_PLUG_BOOL(cond))
#define _PLUG_IF_(b)   _PLUG_CAT(_PLUG_IF_, b)
#define _PLUG_IF_0(...)
#define _PLUG_IF_1(...) __VA_ARGS__

/* if  _PLUG_IFNOT(condition)(else) */
#define _PLUG_IFNOT(cond)  _PLUG_IF_(_PLUG_BOOL(cond))
#define _PLUG_IFNOT_(b)    _PLUG_CAT(_PLUG_IF_, b)
#define _PLUG_IFNOT_0(...) __VA_ARGS__
#define _PLUG_IFNOT_1(...)

/* if  _PLUG_IF_ELSE(condition)(then,else) */
#define _PLUG_IF_ELSE(cond)         _PLUG_IF_ELSE_(_PLUG_BOOL(cond))
#define _PLUG_IF_ELSE_(b)           _PLUG_CAT(_PLUG_IF_ELSE_, b)
#define _PLUG_IF_ELSE_1(then, else) then
#define _PLUG_IF_ELSE_0(then, else) else

#define _PLUG_DEFINE_EXTENSION_NAME(name) _PLUG_CAT3(g_, name, _extension)

/* if 'plug_none'  is passed return NULL, else return pointer to the plug extension struct */
#define _PLUG_EXTENSION_PTR(v)                                           \
    _PLUG_IF_ELSE(_PLUG_IS_PROBE(_PLUG_CAT(_PLUG_DEFINE_EXTENSION_, v))) \
    (NULL, &_PLUG_DEFINE_EXTENSION_NAME(v))
#define _PLUG_DEFINE_EXTENSION_plug_none _PLUG_PROBE()

#define _PLUG_RULE(METHODS, PATTERN, PLUG, FLAGS, OPTIONS) \
    _PLUG_RULE_(METHODS, PATTERN, _PLUG_EXTENSION_PTR(PLUG), FLAGS, OPTIONS)

#define _PLUG_RULE_(METHODS, PATTERN, PLUG, FLAGS, OPTIONS) \
    ((plug_rule_t){                                         \
        .methods = (METHODS), .pattern = PATTERN, .plug = PLUG, .flags = (FLAGS), .options = (void *)(OPTIONS)}) //

/* generate callback pointer types based on the list */
#define _PLUG_GEN_CB_TYPE(cbname, ...) _PLUG_CAT3(plug_, cbname, _cb_t)
#define _PLUG_GEN_FN_NAME(cbname, ...) _PLUG_CAT(cbname, _fn)

#define _PLUG_GEN_STRUCT_MEMBER(_, cbname, ...)          \
    _PLUG_GEN_CB_TYPE(cbname) _PLUG_GEN_FN_NAME(cbname); \
    //

#define _PLUG_ABSTRACT_STATE_ARGS(...) _PLUG_IF(_PLUG_HAS_ARGS(__VA_ARGS__))(_PLUG_ABSTRACT_STATE_ARGS_(__VA_ARGS__))
#define _PLUG_ABSTRACT_STATE_ARGS_(firstarg, ...)  \
    _PLUG_IF_ELSE(_PLUG_IS_PLUG_STATE_P(firstarg)) \
    (_PLUG_ABSTRACT_PLUG_STATE_P(firstarg), firstarg) _PLUG_IF(_PLUG_HAS_ARGS(__VA_ARGS__))(, __VA_ARGS__) //

#define _PLUG_GEN_CB_TYPESPEC(_, cbname, returntype, ...)                                    \
    typedef returntype (*_PLUG_GEN_CB_TYPE(cbname))(_PLUG_ABSTRACT_STATE_ARGS(__VA_ARGS__)); \
    //

#define _PLUG_EXTENSION_PREFIX(name) _PLUG_CAT(name, _)

#define _PLUG_EXTENSION_PROTOTYPE(name) const plug_extension_t _PLUG_DEFINE_EXTENSION_NAME(name)
//

#define _PLUG_GEN_CB_SYMBOL_(fn, returntype, ...) \
    __WEAK returntype fn(__VA_ARGS__);            \
    //

#define _PLUG_GEN_CB_SYMBOL(prefix, cbname, returntype, ...)                 \
    _PLUG_GEN_CB_SYMBOL_(_PLUG_CAT(prefix, cbname), returntype, __VA_ARGS__) \
    //

#define _PLUG_EXTENSION_WEAK_SYMBOLS(name)                      \
    _PLUG_EXTENSION_WEAK_SYMBOLS_(_PLUG_EXTENSION_PREFIX(name)) \
    //
#define _PLUG_EXTENSION_WEAK_SYMBOLS_(prefix)       \
    PLUG_CALLBACK_LIST(prefix, _PLUG_GEN_CB_SYMBOL) \
    //

#define _PLUG_EXTENSION_CB_STRUCT_MEMBERS(name)                      \
    _PLUG_EXTENSION_CB_STRUCT_MEMBERS_(_PLUG_EXTENSION_PREFIX(name)) \
//
#define _PLUG_EXTENSION_CB_STRUCT_MEMBERS_(prefix) PLUG_CALLBACK_LIST(prefix, _PLUG_GEN_EXTENSION_CB_STRUCT_MEMBER)
//

#define _PLUG_GEN_EXTENSION_CB_STRUCT_MEMBER(prefix, cbname, ...) \
    ._PLUG_GEN_FN_NAME(cbname) = (_PLUG_GEN_CB_TYPE(cbname))_PLUG_CAT(prefix, cbname), //

#define _PLUG_GEN_EXTENSION_FIELDS_SYMBOL(name)  _PLUG_GEN_EXTENSION_FIELDS_SYMBOL_(_PLUG_EXTENSION_PREFIX(name))
#define _PLUG_GEN_EXTENSION_FIELDS_SYMBOL_(name) _PLUG_CAT3(s_, name, fields)

#define _PLUG_GEN_EXTENSION_FIELDS_INITIALIZER(name, ...) \
    _PLUG_IF(_PLUG_HAS_ARGS(__VA_ARGS__))                 \
    (static const plug_header_t *const _PLUG_GEN_EXTENSION_FIELDS_SYMBOL(name)[] = {__VA_ARGS__, NULL};) //

#define _PLUG_GEN_EXTENSION_STRUCT_INITIALIZER(name, ...)                                                    \
    {                                                                                                        \
        _PLUG_EXTENSION_CB_STRUCT_MEMBERS(name).fields_vec =                                                 \
            _PLUG_IF_ELSE(_PLUG_HAS_ARGS(__VA_ARGS__))((&_PLUG_GEN_EXTENSION_FIELDS_SYMBOL(name)[0]), NULL), \
    }                                                                                                        \
    //

#define _PLUG_EXTENSION(name, ...)             \
    _PLUG_IF_ELSE(_PLUG_HAS_ARGS(__VA_ARGS__)) \
    (_PLUG_EXTENSION_(name, __VA_ARGS__), _PLUG_EXTENSION_(name, void, __VA_ARGS__)) //
/* _PLUG_EXTENSION_(name,_PLUG_IFNOT(_PLUG_HAS_ARGS(__VA_ARGS__))(void,__VA_ARGS__)\ */
#define _PLUG_EXTENSION_(name, type, ...)                                                                       \
    typedef type *plug_state_p;                                                                                 \
    _PLUG_IF_ELSE(_PLUG_IS_VOID(type))                                                                          \
    (, STATIC_ASSERT(sizeof(type) <= sizeof(abstract_plug_state_t), "type larger than abstract_plug_state_t");) \
        _PLUG_EXTENSION_WEAK_SYMBOLS(name) _PLUG_GEN_EXTENSION_FIELDS_INITIALIZER(name, __VA_ARGS__)            \
            const plug_extension_t                                                                              \
            _PLUG_DEFINE_EXTENSION_NAME(name) = _PLUG_GEN_EXTENSION_STRUCT_INITIALIZER(name, __VA_ARGS__)
//

#define _PLUG_EXTENSION_CALLBACK_TYPES() PLUG_CALLBACK_LIST(_, _PLUG_GEN_CB_TYPESPEC)

#define _PLUG_EXTENSION_STRUCT_MEMBERS()           \
    PLUG_CALLBACK_LIST(_, _PLUG_GEN_STRUCT_MEMBER) \
    const plug_header_t *const *fields_vec;        \
    //

#define _PLUG_HEADER_T_INITIALIZER(regname) \
    {                                       \
        .fhash = regname.fhash,             \
    }

#define _PLUG_EXTENSION_REGISTER_HEADER(regname, fieldhash, valsize, fieldsize)                          \
    static struct _PLUG_CAT(regname,_data)                                                               \
    {                                                                                                    \
        bool is_set;                                                                                     \
        _PLUG_IF(_PLUG_BOOL(valsize))(char value[valsize];)                                              \
        _PLUG_IF(_PLUG_BOOL(fieldsize))(char field[fieldsize]);                                          \
    } _PLUG_CAT(static_, regname)      = {.is_set = false,                                               \
                                          _PLUG_IF(_PLUG_BOOL(valsize))(.value = {0}, )                  \
                                              _PLUG_IF(_PLUG_BOOL(fieldsize))(.field = {0}, )};          \
    static const plug_header_t regname = {                                                               \
        .fhash      = fieldhash,                                                                         \
        .is_set     = &(_PLUG_CAT(static_, regname).is_set),                                             \
        .value_ptr  = _PLUG_IF_ELSE(_PLUG_NOT(valsize))(NULL, _PLUG_CAT(static_, regname).value),        \
        .value_size = _PLUG_IF_ELSE(_PLUG_NOT(valsize))(0, sizeof(_PLUG_CAT(static_, regname).value)),   \
        .field_ptr  = _PLUG_IF_ELSE(_PLUG_NOT(fieldsize))(NULL, _PLUG_CAT(static_, regname).field),      \
        .field_size = _PLUG_IF_ELSE(_PLUG_NOT(fieldsize))(0, sizeof(_PLUG_CAT(static_, regname).field)), \
    }

#endif
