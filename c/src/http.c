#include "http.h"
#include "raygui.h"
#include <limits.h>
#include <unistd.h>
int calc_calculate(const char *expr, double *out, char *err, size_t errlen);
struct conn_state {
  char *body;
  size_t len;
  size_t cap;
};

struct file_entry {
  const char *url;
  const char *name;
  const char *mime;
};

typedef int (*api_handler)(struct MHD_Connection *connection, const char *body, size_t body_len, char **out, size_t *out_len);
struct api_entry {
  const char *path;
  const char *mime;
  api_handler handler;
};

static const struct file_entry files[] = {
  {"/", "index.html", "text/html; charset=utf-8"},
  {"/index.html", "index.html", "text/html; charset=utf-8"},
  {"/calculator.css", "calculator.css", "text/css; charset=utf-8"},
  {"/script.js", "script.js", "text/javascript; charset=utf-8"},
};

static const char *api_arg(struct MHD_Connection *connection, const char *key) {
  const char *value = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, key);
  return value ? value : "";
}

static int api_calc(struct MHD_Connection *connection, const char *body, size_t body_len, char **out, size_t *out_len) {
  const char *expr = body ? body : "";
  char err[256];
  char result[320];
  double value = 0.0;
  (void) connection;
  (void) body_len;
  err[0] = '\0';
  if (calc_calculate(expr, &value, err, sizeof(err)) != 0) {
    snprintf(result, sizeof(result), "{\"error\":\"%s\"}", err[0] ? err : "calculation failed");
    *out = strdup(result);
    *out_len = strlen(*out);
    return MHD_HTTP_BAD_REQUEST;
  }
  snprintf(result, sizeof(result), "{\"result\":%.12g}", value);
  *out = strdup(result);
  *out_len = strlen(*out);
  return MHD_HTTP_OK;
}

static const struct api_entry api_routes[] = {
  {"/api/calc", "application/json; charset=utf-8", api_calc},
};

static const char *exe_dir(void) {
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

static int read_file(const char *path, char **out, size_t *out_len) {
  FILE *file = fopen(path, "rb");
  long size;
  char *buffer;
  size_t got;
  if (!file) return 0;
  if (fseek(file, 0, SEEK_END) != 0) { fclose(file); return 0; }
  size = ftell(file);
  if (size < 0) { fclose(file); return 0; }
  rewind(file);
  buffer = malloc((size_t) size + 1);
  if (!buffer) { fclose(file); return 0; }
  got = fread(buffer, 1, (size_t) size, file);
  fclose(file);
  buffer[got] = '\0';
  *out = buffer;
  *out_len = got;
  return 1;
}

static enum MHD_Result respond(struct MHD_Connection *connection, unsigned int status, const char *mime, char *body, size_t len) {
  struct MHD_Response *response;
  enum MHD_Result ret;
  if (!body) {
    body = strdup("");
    len = 0;
  }
  response = MHD_create_response_from_buffer(len, body, MHD_RESPMEM_MUST_FREE);
  if (!response) {
    free(body);
    return MHD_NO;
  }
  MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, mime);
  ret = MHD_queue_response(connection, status, response);
  MHD_destroy_response(response);
  return ret;
}

static enum MHD_Result serve_error(struct MHD_Connection *connection, unsigned int status) {
  char *body;
  const char *text = (status == MHD_HTTP_NOT_FOUND) ? "not found" : "error";
  body = strdup(text);
  return respond(connection, status, "text/plain; charset=utf-8", body, strlen(body));
}

static enum MHD_Result dispatch(struct MHD_Connection *connection, const char *url, const char *method, struct conn_state *state) {
  size_t i;
  int is_get = (strcmp(method, "GET") == 0) || (strcmp(method, "HEAD") == 0);
  int is_post = (strcmp(method, "POST") == 0);
  if (is_get || is_post) {
    for (i = 0; i < sizeof(api_routes) / sizeof(api_routes[0]); i++) {
      if (strcmp(url, api_routes[i].path) == 0) {
        char *body = NULL;
        size_t len = 0;
        int status = api_routes[i].handler(connection, state->body, state->len, &body, &len);
        return respond(connection, (unsigned int) status, api_routes[i].mime, body, len);
      }
    }
  }

  if (!is_get) return serve_error(connection, MHD_HTTP_METHOD_NOT_ALLOWED);
  for (i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
    if (strcmp(url, files[i].url) == 0) {
      char path[PATH_MAX];
      char *body = NULL;
      size_t len = 0;
      snprintf(path, sizeof(path), "%s/%s", exe_dir(), files[i].name);
      if (!read_file(path, &body, &len)) return serve_error(connection, MHD_HTTP_NOT_FOUND);
      if (strcmp(method, "HEAD") == 0) {
        free(body);
        body = NULL;
        len = 0;
      }
      return respond(connection, MHD_HTTP_OK, files[i].mime, body, len);
    }
  }

  return serve_error(connection, MHD_HTTP_NOT_FOUND);
}

static enum MHD_Result ahc_echo(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **ptr) {
  struct conn_state *state = (struct conn_state *) *ptr;
  (void) cls;
  (void) version;
  if (!state) {
    state = calloc(1, sizeof(*state));
    if (!state) return MHD_NO;
    *ptr = state;
    return MHD_YES;
  }

  if (*upload_data_size != 0) {
    char *grown = realloc(state->body, state->len + *upload_data_size + 1);
    if (!grown) return MHD_NO;
    state->body = grown;
    memcpy(state->body + state->len, upload_data, *upload_data_size);
    state->len += *upload_data_size;
    state->body[state->len] = '\0';
    *upload_data_size = 0;
    return MHD_YES;
  }

  return dispatch(connection, url, method, state);
}

static void request_completed(void *cls, struct MHD_Connection *connection, void **con_cls, enum MHD_RequestTerminationCode toe) {
  struct conn_state *state = (struct conn_state *) *con_cls;
  (void) cls;
  (void) connection;
  (void) toe;
  if (state) {
    free(state->body);
    free(state);
    *con_cls = NULL;
  }
}

int http_main(void) {
  struct MHD_Daemon *d;
  d = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_THREAD_PER_CONNECTION, 8080, NULL, NULL, &ahc_echo, NULL, MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL, MHD_OPTION_END);
  if (NULL == d) return 1;
  raygui_main(); //neo: keep MHD running and start raygui.c
  MHD_stop_daemon(d);
  return 0;
}
