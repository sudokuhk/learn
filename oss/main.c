//
//  main.c
//  Learn
//
//  Created by huangkun on 3/20/17.
//
//

#include <stdio.h>
#include <string.h>

#include "oss_sdk.h"
#include "php_sdk.h"

int main()
{
    array * parray = NULL;
    
#if 0
    oss_sdk_download("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                     "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                     "test/huangkun/test.m3u8",
                     "mydownload0.m3u8", NULL);
    
#endif
#if 0
    oss_sdk_download("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                     "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                     "test/huangkun/test.m3u8",
                     "mydownload1.m3u8", NULL);
#endif
    
#if 0
    oss_sdk_download("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                     "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                     "common/other/188_F775b66f03c37967859B.mkv",
                     "test.mkv", NULL);
#endif
    
#if 0
    oss_sdk_upload("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                   "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                   "test/huangkun/myupload.m3u8",
                   "mydownload0.m3u8",
                   "application/vnd.apple.mpegurl", NULL);
    
    closesock(&oss_sock);
    
    oss_sdk_download("http://oss-cn-shenzhen.aliyuncs.com", "pingqu",
                     "AOfJGJvJfgcy1Onu", "KZnJ3XPTFePbTCV4Nd4ldLFAlCovlQ",
                     "test/huangkun/test.m3u8",
                     "mydownload1.m3u8", NULL);
#endif
    
    parray = create_array(1024);
    do_php_post_request("http://api.pingqulive.ping-qu.com:80", "/v2_1/tc/job/task",
            "application/x-www-form-urlencoded; charset=utf-8",
                        "worker=xxxxx", strlen("worker=xxxxx"), parray);
    printf("res:%s", (const char *)parray->data);
    
    destroy_array(&parray);

    return 0;
}
