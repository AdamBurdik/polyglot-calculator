#define _POSIX_C_SOURCE 200809L
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "http.h"
#include "jvm.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define CALC_MAX_OPS 64
#define CALC_MAX_NAME 32
#define CALC_MAX_STACK 256
#define CALC_MAX_DEPTH 32
#define CALC_MAX_PLAN 4096
#define CALC_ERR 256
typedef struct calc_ctx calc_ctx;
typedef int (*calc_op_fn)(calc_ctx *ctx, const double *args, size_t argc, double *out);
struct calc_ctx {
  const char *expr;
  int depth;
  char err[CALC_ERR];
};

struct calc_op {
  char name[CALC_MAX_NAME];
  int arity;
  calc_op_fn fn;
};

struct calc_inst {
  int is_op;
  double num;
  char name[CALC_MAX_NAME];
};

struct calc_plan {
  struct calc_inst items[CALC_MAX_PLAN];
  size_t len;
};

static struct calc_op calc_ops[CALC_MAX_OPS];
static size_t calc_op_count;
static pthread_mutex_t calc_py_lock = PTHREAD_MUTEX_INITIALIZER;
static PyObject *calc_py_find_order = NULL;
static int calc_py_state = 0;
static void calc_fail(calc_ctx *ctx, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(ctx->err, sizeof(ctx->err), fmt, ap);
  va_end(ap);
}

// JVM Globals
JavaVM *g_jvm = NULL;
jclass g_main_cls = NULL;
jmethodID g_multiple_mid = NULL;

int calc_register(const char *name, int arity, calc_op_fn fn) {
  size_t i;
  if (!name || !fn || arity < 1 || strlen(name) >= CALC_MAX_NAME) return -1;
  for (i = 0; i < calc_op_count; i++) if (strcmp(calc_ops[i].name, name) == 0) return -1;
  if (calc_op_count >= CALC_MAX_OPS) return -1;
  snprintf(calc_ops[calc_op_count].name, CALC_MAX_NAME, "%s", name);
  calc_ops[calc_op_count].arity = arity;
  calc_ops[calc_op_count].fn = fn;
  calc_op_count++;
  return 0;
}

static const struct calc_op *calc_find(const char *name) {
  size_t i;
  for (i = 0; i < calc_op_count; i++) if (strcmp(calc_ops[i].name, name) == 0) return &calc_ops[i];
  return NULL;
}

static int op_add(calc_ctx *ctx, const double *args, size_t argc, double *out) {
  (void) ctx;
  (void) argc;
  double result;
  // __asm__("vaddsd %1, %2, %0" : "=x"(result) : "x"(args[0]), "x"(args[1])); //neo: yum, assembly.
  __asm__("addsd %1, %0" : "=x"(result) : "x"(args[1]), "0"(args[0])); // hey this asm should work on my machine fr fr
  *out = result;
  return 0;
}

static int op_sub(calc_ctx *ctx, const double *args, size_t argc, double *out) {
  (void) ctx;
  (void) argc;
  *out = args[0] - args[1]; //neo: swap out for a language call
  return 0;
}

// adamix: multiple operation calling java method
static int op_mul(calc_ctx *ctx, const double *args, size_t argc, double *out) {
  (void) ctx;
  (void) argc;

  JNIEnv *env = get_current_env();

  if (g_main_cls == NULL || g_multiple_mid == NULL) {
    fprintf(stderr, "Error: JNI classes or methods not cached yet!\n");
    return 1;
  }

  jdouble result = (*env)->CallStaticDoubleMethod(env, g_main_cls, g_multiple_mid, args[0], args[1]);

  double c = (double) result;

  *out = c;
  return 0;
}

/* static int op_div(calc_ctx *ctx, const double *args, size_t argc, double *out) {
  (void) argc;
  if (args[1] == 0.0) {
    calc_fail(ctx, "division by zero");
    return -1;
  }
  *out = args[0] / args[1]; //neo: swap out for a language call
  return 0;
} */
// D division.
extern int op_div_d(calc_ctx *ctx, const double *args, size_t argc, double *out);
static int op_div(calc_ctx *ctx, const double *args, size_t argc, double *out) { 
  int status = op_div_d(ctx, args, argc, out);
  if (status != 0) {
    calc_fail(ctx, "Ah ah ah! no dividing by 0."); 
    return -1;
  }
  return 0;
}
// end of D division
static int op_pow(calc_ctx *ctx, const double *args, size_t argc, double *out) {
  (void) ctx;
  (void) argc;
  *out = pow(args[0], args[1]); //neo: swap out for a language call
  return 0;
}

static int op_neg(calc_ctx *ctx, const double *args, size_t argc, double *out) {
  (void) ctx;
  (void) argc;
  *out = -args[0]; //neo: DONT swap out for a language call, we need to have at least one operation done in C
  return 0;
}

static int op_sqrt(calc_ctx *ctx, const double *args, size_t argc, double *out) {
  (void) argc;
  if (args[0] < 0.0) {
    calc_fail(ctx, "sqrt of a negative number");
    return -1;
  }
  *out = sqrt(args[0]); //neo: swap out for a language call
  return 0;
}

static void calc_init(void) { //neo: maps each python-outptted operation to C functions
  calc_register("add", 2, op_add); //neo: calc_register("op_name", op_arity_or_number_of_numbers, C_function)
  calc_register("sub", 2, op_sub);
  calc_register("mul", 2, op_mul);
  calc_register("div", 2, op_div);
  calc_register("pow", 2, op_pow);
  calc_register("neg", 1, op_neg);
  calc_register("sqrt", 1, op_sqrt);
}

static const char *calc_exe_dir(void) {
  static char dir[PATH_MAX];
  static int ready;
  if (!ready) {
    char path[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n > 0) {
      char *slash;
      path[n] = '\0';
      slash = strrchr(path, '/');
      if (slash) *slash = '\0';
      snprintf(dir, sizeof(dir), "%s", path);
    } else {
      snprintf(dir, sizeof(dir), ".");
    }
    ready = 1;
  }
  return dir;
}

static int calc_find_script(char *out, size_t cap) {
  const char *candidates[] = {"find_order.py", "../python/find_order.py"};
  char path[PATH_MAX];
  size_t i;
  for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    snprintf(path, sizeof(path), "%s/%s", calc_exe_dir(), candidates[i]);
    if (access(path, R_OK) == 0) {
      snprintf(out, cap, "%s", path);
      return 1;
    }
  }
  if (access("python/find_order.py", R_OK) == 0) {
    snprintf(out, cap, "%s", "python/find_order.py");
    return 1;
  }
  if (access("find_order.py", R_OK) == 0) {
    snprintf(out, cap, "%s", "find_order.py");
    return 1;
  }
  return 0;
}

static void calc_py_error(char *err, size_t errlen) {
  PyObject *type = NULL, *value = NULL, *tb = NULL;
  PyErr_Fetch(&type, &value, &tb);
  if (!value) {
    snprintf(err, errlen, "python error");
  } else {
    PyObject *s = PyObject_Str(value);
    const char *msg = s ? PyUnicode_AsUTF8(s) : NULL;
    snprintf(err, errlen, "%s", msg ? msg : "python error");
    Py_XDECREF(s);
  }
  Py_XDECREF(type);
  Py_XDECREF(value);
  Py_XDECREF(tb);
}

static int calc_python_init(char *err, size_t errlen) {
  char script[PATH_MAX], dir[PATH_MAX];
  char *slash;
  PyObject *module = NULL;
  PyObject *dir_obj = NULL;
  PyObject *sys_path = NULL;
  if (calc_py_state) return calc_py_state == 1 ? 0 : -1;
  if (!calc_find_script(script, sizeof(script))) {
    snprintf(err, errlen, "find_order.py not found");
    calc_py_state = -1;
    return -1;
  }
  snprintf(dir, sizeof(dir), "%s", script);
  slash = strrchr(dir, '/');
  if (slash) *slash = '\0';
  else snprintf(dir, sizeof(dir), ".");
  Py_Initialize();
  sys_path = PySys_GetObject("path");
  if (sys_path) {
    dir_obj = PyUnicode_FromString(dir);
    if (dir_obj) {
      PyList_Insert(sys_path, 0, dir_obj);
      Py_DECREF(dir_obj);
      dir_obj = NULL;
    }
  }
  module = PyImport_ImportModule("find_order");
  if (!module) {
    calc_py_error(err, errlen);
    PyEval_SaveThread();
    calc_py_state = -1;
    return -1;
  }
  calc_py_find_order = PyObject_GetAttrString(module, "find_order");
  Py_DECREF(module);
  if (!calc_py_find_order || !PyCallable_Check(calc_py_find_order)) {
    snprintf(err, errlen, "find_order.py does not define find_order()");
    Py_XDECREF(calc_py_find_order);
    calc_py_find_order = NULL;
    PyEval_SaveThread();
    calc_py_state = -1;
    return -1;
  }
  PyEval_SaveThread();
  calc_py_state = 1;
  return 0;
}

static int plan_parse_line(const char *line, size_t len, struct calc_plan *plan, char *err, size_t errlen) {
  if (len == 0) return 0;
  if (line[0] == 'n') {
    char *end = NULL;
    char num[1024];
    if (len >= sizeof(num)) {
      snprintf(err, errlen, "bad number in plan");
      return -1;
    }
    memcpy(num, line, len);
    num[len] = '\0';
    if (num[1] == ' ' || num[1] == '\t') {
      double value = strtod(num + 1, &end);
      if (end == num + 1) {
        snprintf(err, errlen, "bad number in plan: %s", num);
        return -1;
      }
      if (plan->len >= CALC_MAX_PLAN) {
        snprintf(err, errlen, "expression is too large");
        return -1;
      }
      plan->items[plan->len].is_op = 0;
      plan->items[plan->len].num = value;
      plan->items[plan->len].name[0] = '\0';
      plan->len++;
      return 0;
    }
    snprintf(err, errlen, "bad number in plan: %s", num);
    return -1;
  }
  if (line[0] == 'o') {
    const char *name = line;
    char namebuf[CALC_MAX_NAME];
    size_t nlen;
    if (len >= 2 && (name[1] == ' ' || name[1] == '\t')) name += 2;
    else if (len == 1) {
      snprintf(err, errlen, "bad operator in plan");
      return -1;
    } else {
      name += 1;
    }
    while (name < line + len && (*name == ' ' || *name == '\t')) name++;
    nlen = (size_t) (line + len - name);
    if (nlen == 0 || nlen >= CALC_MAX_NAME) {
      snprintf(err, errlen, "bad operator in plan");
      return -1;
    }
    memcpy(namebuf, name, nlen);
    namebuf[nlen] = '\0';
    if (plan->len >= CALC_MAX_PLAN) {
      snprintf(err, errlen, "expression is too large");
      return -1;
    }
    plan->items[plan->len].is_op = 1;
    plan->items[plan->len].num = 0.0;
    snprintf(plan->items[plan->len].name, CALC_MAX_NAME, "%s", namebuf);
    plan->len++;
    return 0;
  }
  if (line[0] == '!') {
    char msg[CALC_ERR];
    size_t mlen;
    const char *s = line + 1;
    while (s < line + len && *s == ' ') s++;
    mlen = (size_t) (line + len - s);
    if (mlen >= CALC_ERR) mlen = CALC_ERR - 1;
    memcpy(msg, s, mlen);
    msg[mlen] = '\0';
    snprintf(err, errlen, "%s", msg);
    return -1;
  }
  snprintf(err, errlen, "unexpected plan line");
  return -1;
}

static int plan_parse(const char *text, struct calc_plan *plan, char *err, size_t errlen) {
  const char *p = text;
  plan->len = 0;
  while (*p) {
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t) (nl - p) : strlen(p);
    while (len > 0 && p[len - 1] == '\r') len--;
    if (len > 0) {
      if (plan_parse_line(p, len, plan, err, errlen) != 0) return -1;
    }
    if (!nl) break;
    p = nl + 1;
  }
  if (plan->len == 0) {
    snprintf(err, errlen, "empty expression");
    return -1;
  }
  return 0;
}

static int find_order(const char *expr, struct calc_plan *plan, char *err, size_t errlen) {
  PyGILState_STATE gil;
  PyObject *arg = NULL, *result = NULL;
  const char *text = NULL;
  int rc;
  pthread_mutex_lock(&calc_py_lock);
  if (calc_python_init(err, errlen) != 0) {
    pthread_mutex_unlock(&calc_py_lock);
    return -1;
  }
  gil = PyGILState_Ensure();
  arg = PyUnicode_FromString(expr);
  if (!arg) {
    calc_py_error(err, errlen);
    PyGILState_Release(gil);
    pthread_mutex_unlock(&calc_py_lock);
    return -1;
  }
  result = PyObject_CallFunctionObjArgs(calc_py_find_order, arg, NULL);
  Py_DECREF(arg);
  if (!result) {
    calc_py_error(err, errlen);
    PyGILState_Release(gil);
    pthread_mutex_unlock(&calc_py_lock);
    return -1;
  }
  text = PyUnicode_AsUTF8(result);
  if (!text) {
    calc_py_error(err, errlen);
    Py_DECREF(result);
    PyGILState_Release(gil);
    pthread_mutex_unlock(&calc_py_lock);
    return -1;
  }
  rc = plan_parse(text, plan, err, errlen);
  Py_DECREF(result);
  PyGILState_Release(gil);
  pthread_mutex_unlock(&calc_py_lock);
  return rc;
}

static int plan_eval(const struct calc_plan *plan, calc_ctx *ctx, double *out) {
  double stack[CALC_MAX_STACK];
  size_t sp = 0;
  size_t i;
  for (i = 0; i < plan->len; i++) {
    const struct calc_inst *inst = &plan->items[i];
    const struct calc_op *op;
    if (!inst->is_op) {
      if (sp >= CALC_MAX_STACK) {
        calc_fail(ctx, "expression is too deep"); //neo: it IS that deep bro
        return -1;
      }
      stack[sp++] = inst->num;
      continue;
    }
    op = calc_find(inst->name);
    if (!op) {
      calc_fail(ctx, "no C function registered for '%s'", inst->name);
      return -1;
    }
    if ((size_t) op->arity > sp) {
      calc_fail(ctx, "'%s' needs %d values", inst->name, op->arity);
      return -1;
    }
    if (op->fn(ctx, &stack[sp - (size_t) op->arity], (size_t) op->arity, &stack[sp - (size_t) op->arity]) != 0) {
      if (ctx->err[0] == '\0') calc_fail(ctx, "'%s' failed", inst->name);
      return -1;
    }
    sp -= (size_t) op->arity - 1;
  }
  if (sp != 1) {
    calc_fail(ctx, "expression left %zu values", sp);
    return -1;
  }
  *out = stack[0];
  return 0;
}

static int calc_eval(const char *expr, double *out, char *err, size_t errlen, int depth) {
  calc_ctx ctx;
  struct calc_plan *plan;
  memset(&ctx, 0, sizeof(ctx));
  ctx.expr = expr;
  ctx.depth = depth;
  if (depth > CALC_MAX_DEPTH) {
    snprintf(err, errlen, "recursion limit reached");
    return -1;
  }
  plan = malloc(sizeof(*plan));
  if (!plan) {
    snprintf(err, errlen, "out of memory");
    return -1;
  }
  if (find_order(expr, plan, ctx.err, sizeof(ctx.err)) != 0) {
    snprintf(err, errlen, "%s", ctx.err);
    free(plan);
    return -1;
  }
  if (plan_eval(plan, &ctx, out) != 0) {
    snprintf(err, errlen, "%s", ctx.err[0] ? ctx.err : "evaluation failed");
    free(plan);
    return -1;
  }
  free(plan);
  return 0;
}

int calc_recalculate(calc_ctx *ctx, const char *expr, double *out) {
  char err[CALC_ERR];
  int rc;
  if (ctx->depth >= CALC_MAX_DEPTH) {
    calc_fail(ctx, "recursion limit reached");
    return -1;
  }
  err[0] = '\0';
  rc = calc_eval(expr, out, err, sizeof(err), ctx->depth + 1);
  if (rc != 0) calc_fail(ctx, "%s", err[0] ? err : "evaluation failed");
  return rc;
}

int calc_calculate(const char *expr, double *out, char *err, size_t errlen) {
  return calc_eval(expr, out, err, errlen, 0);
}

int main(int argc, char **argv) {
  init_jvm();
  calc_init();
  if (argc > 1) {
    double out = 0.0;
    char err[CALC_ERR];
    err[0] = '\0';
    if (calc_calculate(argv[1], &out, err, sizeof(err)) != 0) {
      fprintf(stderr, "error: %s\n", err[0] ? err : "calculation failed");
      return 1;
    }
    printf("%.12g\n", out);
    return 0;
  }
  return http_main(); //neo: give off main to http.c
}
