#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static void * ngx_http_test_header_create_conf(ngx_conf_t *cf);
static char * ngx_http_test_header_merge_conf(ngx_conf_t *cf, void *parent, void *child);

static ngx_int_t ngx_http_test_header_name_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_test_header_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_test_header_init(ngx_conf_t *cf);
static char * ngx_http_test_header_add_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

typedef struct {
    ngx_str_t   test_header_name;
} ngx_http_test_header_conf_t;

static ngx_http_variable_t  ngx_http_test_header_vars[] = {

    { ngx_string("test_header_name"), NULL, ngx_http_test_header_name_variable, 0, 0, 0 },

      ngx_http_null_variable
};

static ngx_command_t ngx_http_test_header_commands[] = {
    { ngx_string("test_header_name"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_test_header_add_conf,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_test_header_conf_t, test_header_name),
      NULL },

      ngx_null_command
};

static ngx_http_module_t ngx_http_test_header_module_ctx = {
    ngx_http_test_header_add_variables,    /* preconfiguration */
    ngx_http_test_header_init,             /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_test_header_create_conf,      /* create location configuration */
    ngx_http_test_header_merge_conf        /* merge location configuration */
};

ngx_module_t ngx_http_test_header_module = {
    NGX_MODULE_V1,
    &ngx_http_test_header_module_ctx,           /* module context */
    ngx_http_test_header_commands,              /* module directives */
    NGX_HTTP_MODULE,                      /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;

static ngx_int_t
ngx_http_test_header_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_test_header_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_test_header_name_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_test_header_conf_t  *conf;
    ngx_list_part_t *part;
    ngx_table_elt_t  *h;
    ngx_uint_t        i;

    conf = ngx_http_get_module_loc_conf(r->main, ngx_http_test_header_module);

    part = &r->headers_out.headers.part;
    h = part->elts;

    for (i = 0; 1; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                h = NULL;
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        if (ngx_strcmp(h[i].key.data, conf->test_header_name.data) == 0) break;
    }

    v->len = conf->test_header_name.len;
    if (h != NULL) {
        v->len += 2 + h[i].value.len;
    }
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;
    if (h == NULL) {
        v->data = conf->test_header_name.data;
    }
    else {
        v->data = ngx_pcalloc(r->pool, v->len);
        if (v->data == NULL) {
            return NGX_ERROR;
        }
        ngx_sprintf(v->data, "%V: %V", &conf->test_header_name, &h[i].value);
    }

    return NGX_OK;
}

static void *
ngx_http_test_header_create_conf(ngx_conf_t *cf)
{
    ngx_http_test_header_conf_t  *conf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0,
                   "init test_header config");

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_test_header_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    return conf;
}


static char *
ngx_http_test_header_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_test_header_conf_t *prev = parent;
    ngx_http_test_header_conf_t *conf = child;
    
    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0,
                   "merge test_header config");

    ngx_conf_merge_str_value(conf->test_header_name, prev->test_header_name, "");

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_add_test_header(ngx_http_request_t *r, ngx_str_t *header_name)
{
    ngx_table_elt_t  *h;
    u_char buf[NGX_INT_T_LEN];

    h = ngx_list_push(&r->headers_out.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->hash = 1;
    h->key = *header_name;

    ngx_sprintf(buf, "%i%Z", rand());

    h->value.len = ngx_strlen(buf);
    h->value.data = ngx_pnalloc(r->pool, h->value.len);
    if (h->value.data == NULL) {
        return NGX_ERROR;
    }
    
    ngx_memcpy(h->value.data, buf, h->value.len);
    
    return NGX_OK;
}

static char *
ngx_http_test_header_add_conf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_test_header_conf_t *hcf = conf;
    ngx_str_t                   *value;

    value = cf->args->elts;

    hcf->test_header_name = value[1];

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_http_test_header_filter(ngx_http_request_t *r)
{
    ngx_http_test_header_conf_t  *conf;

    if (r != r->main) {
        return ngx_http_next_header_filter(r);
    }

    conf = ngx_http_get_module_loc_conf(r, ngx_http_test_header_module);

    if (!conf->test_header_name.len) {
        return ngx_http_next_header_filter(r);
    }

    switch (r->headers_out.status) {

    case NGX_HTTP_OK:
    case NGX_HTTP_CREATED:
    case NGX_HTTP_NO_CONTENT:
    case NGX_HTTP_PARTIAL_CONTENT:
    case NGX_HTTP_MOVED_PERMANENTLY:
    case NGX_HTTP_MOVED_TEMPORARILY:
    case NGX_HTTP_SEE_OTHER:
    case NGX_HTTP_NOT_MODIFIED:
    case NGX_HTTP_TEMPORARY_REDIRECT:
    case NGX_HTTP_PERMANENT_REDIRECT:
        if (ngx_http_add_test_header(r, &conf->test_header_name) != NGX_OK) {
            return NGX_ERROR;
        }
        break;
    }

    return ngx_http_next_header_filter(r);
}


static ngx_int_t
ngx_http_test_header_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_test_header_filter;

    return NGX_OK;
}