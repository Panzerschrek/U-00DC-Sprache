/**
 * @license
 * Copyright 2010 The Emscripten Authors
 * SPDX-License-Identifier: MIT
 */

#if MODULARIZE
var Module = moduleArg;
#elif USE_CLOSURE_COMPILER
// if (!Module)` is crucial for Closure Compiler here as it will
// otherwise replace every `Module` occurrence with the object below
var /** @type{Object} */ Module;
if (!Module) /** @suppress{checkTypes}*/Module = 
#if AUDIO_WORKLET
  globalThis.{{{ EXPORT_NAME }}} || 
#endif
  {"__EMSCRIPTEN_PRIVATE_MODULE_EXPORT_NAME_SUBSTITUTION__":1};

#endif

#if MODULARIZE && EXPORT_READY_PROMISE
// Set up the promise that indicates the Module is initialized
var readyPromiseResolve, readyPromiseReject;
Module['ready'] = new Promise((resolve, reject) => {
  readyPromiseResolve = resolve;
  readyPromiseReject = reject;
});
#if ASSERTIONS
{{{ addReadyPromiseAssertions("Module['ready']") }}}
#endif
#endif

#if AUDIO_WORKLET
var ENVIRONMENT_IS_AUDIO_WORKLET = typeof AudioWorkletGlobalScope !== 'undefined';
#endif

#if ASSERTIONS || PTHREADS
var ENVIRONMENT_IS_WEB = true;
#endif // ASSERTIONS || PTHREADS

#if WASM_WORKERS
var ENVIRONMENT_IS_WASM_WORKER = Module['$ww'];
#endif


// Redefine these in a --pre-js to override behavior. If you would like to
// remove out() or err() altogether, you can no-op it out to function() {},
// and build with --closure 1 to get Closure optimize out all the uses
// altogether.

var out = (text) => console.log(text);
var err = (text) => console.error(text);

// Override this function in a --pre-js file to get a signal for when
// compilation is ready. In that callback, call the function run() to start
// the program.
function ready() {
#if MODULARIZE && EXPORT_READY_PROMISE
  readyPromiseResolve(Module);
#endif // MODULARIZE
#if INVOKE_RUN && HAS_MAIN
  {{{ runIfMainThread("run();") }}}
#elif ASSERTIONS
  out('ready() called, and INVOKE_RUN=0. The runtime is now ready for you to call run() to invoke application _main(). You can also override ready() in a --pre-js file to get this signal as a callback')
#endif
#if PTHREADS
  // This Worker is now ready to host pthreads, tell the main thread we can proceed.
  if (ENVIRONMENT_IS_PTHREAD) {
    startWorker(Module);
  }
#endif
}

// --pre-jses are emitted after the Module integration code, so that they can
// refer to Module (if they choose; they can also define Module)
{{{ preJS() }}}

#if PTHREADS

#if !MODULARIZE
// In MODULARIZE mode _scriptDir needs to be captured already at the very top of the page immediately when the page is parsed, so it is generated there
// before the page load. In non-MODULARIZE modes generate it here.
var _scriptDir = (typeof document != 'undefined' && document.currentScript) ? document.currentScript.src : undefined;
#endif

// MINIMAL_RUNTIME does not support --proxy-to-worker option, so Worker and Pthread environments
// coincide.
#if WASM_WORKERS
var ENVIRONMENT_IS_WORKER = typeof importScripts == 'function',
  ENVIRONMENT_IS_PTHREAD = ENVIRONMENT_IS_WORKER && !ENVIRONMENT_IS_WASM_WORKER;
#else
var ENVIRONMENT_IS_WORKER = ENVIRONMENT_IS_PTHREAD = typeof importScripts == 'function';
#endif

var currentScriptUrl = typeof _scriptDir != 'undefined' ? _scriptDir : ((typeof document != 'undefined' && document.currentScript) ? document.currentScript.src : undefined);
#endif // PTHREADS
