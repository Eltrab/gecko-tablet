/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jsfun_h___
#define jsfun_h___
/*
 * JS function definitions.
 */
#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C

struct JSFunction {
    JSObject     *object;       /* back-pointer to GC'ed object header */
    uint16       nargs;         /* maximum number of specified arguments,
                                   reflected as f.length/f.arity */
    uint16       flags;         /* bound method and other flags, see jsapi.h */
    union {
        struct {
            uint16   extra;     /* number of arg slots for local GC roots */
            uint16   minargs;   /* minimum number of specified arguments, used
                                   only when calling fast native */
            JSNative native;    /* native method pointer or null */
        } n;
        struct {
            uint16   nvars;     /* number of local variables */
            uint16   spare;     /* reserved for future use */
            JSScript *script;   /* interpreted bytecode descriptor or null */
        } i;
    } u;
    JSAtom       *atom;         /* name for diagnostics and decompiling */
    JSClass      *clasp;        /* if non-null, constructor for this class */
};

#define JSFUN_EXPR_CLOSURE   0x4000 /* expression closure: function(x)x*x */
#define JSFUN_INTERPRETED    0x8000 /* use u.i if set, u.n if unset */

#define JSFUN_SCRIPT_OR_FAST_NATIVE (JSFUN_INTERPRETED | JSFUN_FAST_NATIVE)

#define FUN_INTERPRETED(fun) ((fun)->flags & JSFUN_INTERPRETED)
#define FUN_SLOW_NATIVE(fun) (!((fun)->flags & JSFUN_SCRIPT_OR_FAST_NATIVE))
#define FUN_SCRIPT(fun)      (FUN_INTERPRETED(fun) ? (fun)->u.i.script : NULL)
#define FUN_NATIVE(fun)      (FUN_SLOW_NATIVE(fun) ? (fun)->u.n.native : NULL)
#define FUN_FAST_NATIVE(fun) (((fun)->flags & JSFUN_FAST_NATIVE)              \
                              ? (JSFastNative) (fun)->u.n.native              \
                              : NULL)
#define FUN_MINARGS(fun)     (((fun)->flags & JSFUN_FAST_NATIVE)              \
                              ? (fun)->u.n.minargs                            \
                              : (fun)->nargs)

extern JSClass js_ArgumentsClass;
extern JSClass js_CallClass;

/* JS_FRIEND_DATA so that VALUE_IS_FUNCTION is callable from the shell. */
extern JS_FRIEND_DATA(JSClass) js_FunctionClass;

/*
 * NB: jsapi.h and jsobj.h must be included before any call to this macro.
 */
#define VALUE_IS_FUNCTION(cx, v)                                              \
    (!JSVAL_IS_PRIMITIVE(v) &&                                                \
     OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_FunctionClass)

/*
 * Macro to access the private slot of the function object after the slot is
 * initialized.
 */
#define GET_FUNCTION_PRIVATE(cx, funobj)                                      \
    (JS_ASSERT(OBJ_GET_CLASS(cx, funobj) == &js_FunctionClass),               \
     (JSFunction *) OBJ_GET_PRIVATE(cx, funobj))

extern JSObject *
js_InitFunctionClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitArgumentsClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_InitCallClass(JSContext *cx, JSObject *obj);

extern JSFunction *
js_NewFunction(JSContext *cx, JSObject *funobj, JSNative native, uintN nargs,
               uintN flags, JSObject *parent, JSAtom *atom);

extern void
js_FinalizeFunction(JSContext *cx, JSFunction *fun);

extern JSObject *
js_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSObject *parent);

extern JSBool
js_LinkFunctionObject(JSContext *cx, JSFunction *fun, JSObject *object);

extern JSFunction *
js_DefineFunction(JSContext *cx, JSObject *obj, JSAtom *atom, JSNative native,
                  uintN nargs, uintN flags);

/*
 * Flags for js_ValueToFunction and js_ReportIsNotFunction.  We depend on the
 * fact that JSINVOKE_CONSTRUCT (aka JSFRAME_CONSTRUCTING) is 1, and test that
 * with #if/#error in jsfun.c.
 */
#define JSV2F_CONSTRUCT         JSINVOKE_CONSTRUCT
#define JSV2F_ITERATOR          JSINVOKE_ITERATOR
#define JSV2F_SEARCH_STACK      0x10000

extern JSFunction *
js_ValueToFunction(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_ValueToFunctionObject(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_ValueToCallableObject(JSContext *cx, jsval *vp, uintN flags);

extern void
js_ReportIsNotFunction(JSContext *cx, jsval *vp, uintN flags);

extern JSObject *
js_GetCallObject(JSContext *cx, JSStackFrame *fp, JSObject *parent);

extern JSBool
js_PutCallObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_GetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_SetCallVariable(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JSBool
js_GetArgsValue(JSContext *cx, JSStackFrame *fp, jsval *vp);

extern JSBool
js_GetArgsProperty(JSContext *cx, JSStackFrame *fp, jsid id, jsval *vp);

extern JSObject *
js_GetArgsObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_PutArgsObject(JSContext *cx, JSStackFrame *fp);

extern JSBool
js_XDRFunction(JSXDRState *xdr, JSObject **objp);

typedef enum JSLocalKind {
    JSLOCAL_NONE,
    JSLOCAL_ARG,
    JSLOCAL_VAR,
    JSLOCAL_CONST
} JSLocalKind;

extern JSBool
js_AddLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, JSLocalKind kind);

/*
 * Look up an argument or variable name returning its kind when found or
 * JSLOCAL_NONE when no such name exists. When indexp is not null and the name
 * exists, *indexp will receive the index of the corresponding argument or
 * variable.
 */
extern JSLocalKind
js_LookupLocal(JSContext *cx, JSFunction *fun, JSAtom *atom, uintN *indexp);

/*
 * Get names of arguments and variables for the interpreted function.
 *
 * The result is an array allocated from the given pool with
 *   fun->nargs + fun->u.i.nvars
 * elements with the names of the arguments coming first. The argument
 * name is null when it corresponds to a destructive pattern.
 *
 * When bitmap is not null, on successful return it will contain a bit array
 * where for each index below fun->nargs the bit is set when the corresponding
 * argument name is not null. For indexes greater or equal fun->nargs the bit
 * is set when the corresponding var is really a const.
 */
extern JSAtom **
js_GetLocalNames(JSContext *cx, JSFunction *fun, JSArenaPool *pool,
                 uint32 **bitmap);

/*
 * Pseudo-getter function pointers to distinguish the kind of the hidden
 * property.
 */
#define JS_HIDDEN_ARG_GETTER    ((JSPropertyOp) sizeof(jsuword))
#define JS_HIDDEN_VAR_GETTER    ((JSPropertyOp) (2 * sizeof(jsuword)))

JS_END_EXTERN_C

#endif /* jsfun_h___ */
