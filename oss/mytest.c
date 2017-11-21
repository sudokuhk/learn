/*************************************************************************
    > File Name: mytest.c
    > Author: sudoku.huang
    > Mail: sudoku.huang@gmail.com 
    > Created Time: Thu 16 Mar 2017 01:08:42 PM CST
 ************************************************************************/

#include<stdio.h>
#include <oss_c_sdk/oss_api.h>
#include <oss_c_sdk/aos_http_io.h>

int download() 
{
    aos_pool_t *p;
    oss_request_options_t *options;
    aos_status_t *s;
    aos_table_t *headers;
    aos_table_t *params;
    aos_table_t *resp_headers;

    aos_string_t bucket;
    aos_string_t object;
    aos_string_t file;
    
    aos_pool_create(&p, NULL);

    options = oss_request_options_create(p);
    options->config = oss_config_create(options->pool);
    options->ctl = aos_http_controller_create(options->pool, 0);
    
    aos_str_set(&options->config->endpoint, "http://oss-cn-shenzhen.aliyuncs.com");
    aos_str_set(&options->config->access_key_id, "AOfJGJvJfgcy1Onu");
    aos_str_set(&options->config->access_key_secret, "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ");
    aos_str_set(&bucket, "pingqu");
    aos_str_set(&object, "test/huangkun/188_F775b66f03c37967859B/188_F775b66f03c37967859B.m3u8");
    
    aos_str_set(&file, "download.m3u8"); 
    
    headers = aos_table_make(p, 0);
    params = aos_table_make(p, 0);

    s = oss_get_object_to_file(options, &bucket, &object,
                               headers, params, &file, &resp_headers);
    if (aos_status_is_ok(s)) {
        printf("get object succeeded\n");
    } else {
        printf("get object failed, code:%d, %s:%s:%s\n",
               s->code, s->error_code, s->error_msg, s->req_id);
    }
    
    aos_pool_destroy(p);


    return 0;
}

int upload()
{
    aos_pool_t *p = NULL;
    aos_string_t bucket;
    aos_string_t object;
    aos_table_t *headers = NULL;
    aos_table_t *resp_headers = NULL;
    oss_request_options_t *options = NULL;
    aos_status_t *s = NULL;
    aos_string_t file;
    
    aos_pool_create(&p, NULL);

    options = oss_request_options_create(p);
    options->config = oss_config_create(options->pool);
    options->ctl = aos_http_controller_create(options->pool, 0);
    
    aos_str_set(&options->config->endpoint, "http://oss-cn-shenzhen.aliyuncs.com");
    aos_str_set(&options->config->access_key_id, "AOfJGJvJfgcy1Onu");
    aos_str_set(&options->config->access_key_secret, "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ");
    aos_str_set(&bucket, "pingqu");
    aos_str_set(&object, "test/huangkun/188_F775b66f03c37967859B/188_F775b66f03c37967859B.m3u8");
    aos_str_set(&file, "download.m3u8");

    headers = aos_table_make(options->pool, 1);
    apr_table_set(headers, OSS_CONTENT_TYPE, "application/vnd.apple.mpegurl");

    s = oss_put_object_from_file(options, &bucket, &object, &file,
                                 headers, &resp_headers);

    if (!aos_status_is_ok(s)) {
        printf("put object failed, code:%d, %s:%s:%s\n",
               s->code, s->error_code, s->error_msg, s->req_id);
    }
    
    aos_pool_destroy(p);
    
    return 0;
}

int main()
{
    if (aos_http_io_initialize(NULL, 0) != AOSE_OK) {
        return -1;
    }
    upload();
    aos_http_io_deinitialize();

    return 0;
}
