/*
 * scwrap 環境変数にシステム設定を入れてくれるいい感じのアレ
 * 
 * 使い方: scwrap cmd [cmdargs..]
 *
 * 例(希望): sudo scwrap port selfupdate
 */
#include <stdio.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>

static CFURLRef set_proxy_target;

void set_proxy(CFArrayRef proxyList);

void set_proxy_cb(void *rest_untyped, CFArrayRef ps, CFErrorRef error) {
    CFArrayRef rest = (CFArrayRef)CFRetain(rest_untyped);
    
    if(ps == NULL) {
        set_proxy(rest);
    } else {
        set_proxy(ps);
    }

    CFRunLoopStop(CFRunLoopGetCurrent());
    
    CFRelease(rest);
}

void set_proxy(CFArrayRef proxyList) {
    assert(set_proxy_target != NULL);
    if(CFArrayGetCount(proxyList) == 0) {
        return;
    }
    
    CFDictionaryRef proxy = CFArrayGetValueAtIndex(proxyList, 0);
    CFStringRef type = CFDictionaryGetValue(proxy, kCFProxyTypeKey);
    
    if(CFEqual(type, kCFProxyTypeNone)) {
        return;
    } else if(CFEqual(type, kCFProxyTypeAutoConfigurationURL)) {
        CFURLRef pac = CFDictionaryGetValue(proxy, kCFProxyAutoConfigurationURLKey);
        CFMutableArrayRef ps_rest = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, proxyList);
        CFArrayRemoveValueAtIndex(ps_rest, 0);
        
        CFStreamClientContext *ctx = (CFStreamClientContext *)malloc(sizeof(CFStreamClientContext));
        ctx->version = 0;
        ctx->info = ps_rest;
        ctx->retain = (void *(*)(void *))CFRetain;
        ctx->release = (void (*)(void *))CFRelease;
        ctx->copyDescription = (CFStringRef (*)(void *))CFCopyDescription;
        
        CFRunLoopSourceRef rls = CFNetworkExecuteProxyAutoConfigurationURL(pac,
                                                                           set_proxy_target,
                                                                           set_proxy_cb,
                                                                           ctx);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode);
        CFRunLoopRun();
        
        CFRelease(ps_rest);
        return;
    } else if(CFEqual(type, kCFProxyTypeAutoConfigurationJavaScript)) {
        CFStringRef pac = CFDictionaryGetValue(proxy, kCFProxyTypeAutoConfigurationJavaScript);
        CFMutableArrayRef ps_rest = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, proxyList);
        CFArrayRemoveValueAtIndex(ps_rest, 0);
        
        CFStreamClientContext *ctx = (CFStreamClientContext *)malloc(sizeof(CFStreamClientContext));
        ctx->version = 0;
        ctx->info = ps_rest;
        ctx->retain = (void *(*)(void *))CFRetain;
        ctx->release = (void (*)(void *))CFRelease;
        ctx->copyDescription = (CFStringRef (*)(void *))CFCopyDescription;
        
        CFRunLoopSourceRef rls = CFNetworkExecuteProxyAutoConfigurationScript(pac,
                                                                              set_proxy_target,
                                                                              set_proxy_cb,
                                                                              ctx);
        CFRunLoopAddSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode);
        CFRunLoopRun();
        
        CFRelease(ps_rest);
        return;
    } else if(CFEqual(type, kCFProxyTypeFTP)) {
        CFStringRef host = CFDictionaryGetValue(proxy, kCFProxyHostNameKey);
        CFNumberRef port = CFDictionaryGetValue(proxy, kCFProxyPortNumberKey);
        CFStringRef env =  CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
                                                    CFSTR("%@:%@"), host, port);
        char *env_char = (char *)malloc(sizeof(char) * CFStringGetLength(env) + 1);
        
        if(CFStringGetCString(env, env_char, sizeof(char) * CFStringGetLength(env) + 1,
                              kCFStringEncodingUTF8 )) {
            setenv("FTP_PROXY", env_char, 1);
        }
        
        free(env_char);
        CFRelease(env);
        return;
    } else if(CFEqual(type, kCFProxyTypeHTTP)) {
        CFStringRef host = CFDictionaryGetValue(proxy, kCFProxyHostNameKey);
        CFNumberRef port = CFDictionaryGetValue(proxy, kCFProxyPortNumberKey);
        CFStringRef env =  CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
                                                    CFSTR("%@:%@"), host, port);
        char *env_char = (char *)malloc(sizeof(char) * CFStringGetLength(env) + 1);
        
        if(CFStringGetCString(env, env_char, sizeof(char) * CFStringGetLength(env) + 1,
                              kCFStringEncodingUTF8 )) {
            setenv("HTTP_PROXY", env_char, 1);
            setenv("RSYNC_PROXY", env_char, 1);
            setenv("http_proxy", env_char, 1);
        }
        
        free(env_char);
        CFRelease(env);
        return;
    } else if(CFEqual(type, kCFProxyTypeHTTPS)) {
        CFStringRef host = CFDictionaryGetValue(proxy, kCFProxyHostNameKey);
        CFNumberRef port = CFDictionaryGetValue(proxy, kCFProxyPortNumberKey);
        CFStringRef env =  CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
                                                    CFSTR("%@:%@"), host, port);
        char *env_char = (char *)malloc(sizeof(char) * CFStringGetLength(env) + 1);
        
        if(CFStringGetCString(env, env_char, sizeof(char) * CFStringGetLength(env) + 1,
                              kCFStringEncodingUTF8 )) {
            setenv("HTTPS_PROXY", env_char, 1);
        }
        
        free(env_char);
        CFRelease(env);
        return;
    } else if(CFEqual(type, kCFProxyTypeSOCKS)) {
        // muri
        return;
    } else {
        assert(0==1);
    }
}

CFStringRef get_proxy(CFStringRef reach_serv) {
    CFDictionaryRef pset = CFNetworkCopySystemProxySettings();
    CFURLRef reach_serv_url = CFURLCreateWithString(kCFAllocatorDefault,
                                                    reach_serv,
                                                    NULL);
    set_proxy_target = reach_serv_url;
    CFArrayRef ps = CFNetworkCopyProxiesForURL(reach_serv_url, pset);
    
    // CFShow(ps);
    set_proxy(ps);
    
    CFRelease(pset);
    CFRelease(reach_serv_url);
    CFRelease(ps);
}

void environment(CFArrayRef rech_serv_str_array) {
    CFDictionaryRef pset = CFNetworkCopySystemProxySettings();
    
    for(int i = 0; i < CFArrayGetCount(rech_serv_str_array); i++) {
        get_proxy(CFArrayGetValueAtIndex(rech_serv_str_array, i));
    }
    
    CFRelease(pset);
}

int main(int argc, char *argv[]) {
    if(argc == 1) {
        printf("system proxy configuration wrapper\n");
        printf("Useage:\n");
        printf("\tscwrap cmd [args..]\n");
        return -1;
    }
    
    CFStringRef reach_array[] = {
        CFSTR("http://apple.com"),
        CFSTR("https://apple.com"),};
    CFArrayRef reachs = CFArrayCreate (kCFAllocatorDefault,
                                       (const void **)&reach_array,
                                       sizeof(reach_array) / sizeof(*reach_array),
                                       NULL);
    environment(reachs);
    CFRelease(reachs);
    
    execvp(argv[1], &argv[1]);
    
    return 0;
}
