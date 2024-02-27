/*
 * Copyright 2023 NXP 
 *
 * NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be used strictly
 * in accordance with the applicable license terms. By expressly accepting such terms or by downloading,
 * installing, activating and/or otherwise using the software, you are agreeing that you have read,
 * and that you agree to comply with and are bound by, such license terms. If you do not agree to be bound by
 * the applicable license terms, then you may not retain, install, activate or otherwise use the software.
 */

#include "cit_httpd.h"
#include "plug.h"
#include "qmc_features_config.h"

#include <lwip/altcp_tls.h>
#include <lwip/tcpip.h>
#include <core_json.h>
#include "plug_zip_fs.h"
#include "plug_json.h"
#include "json_api.h"

#include "se_session.h"

#include "webservice.h"

#include "json_motor_api_service_task.h"
/*
 * include the zip file containing the web application into the compiled binary.
 * */

PLUG_ZIP_FS_FILE(webroot, "web_root.zip");

#if PLUG_ZIP_FS_FATFS
// serve from sdcard
PLUG_ZIP_FS_FATFS_PATH(webroot_sd, "/web_root.zip");
#endif

/* configuration struct for the web server */
static struct cit_httpd_config s_httpd_config = {0};

static const plug_rule_t s_qmc_webservice_rules[] = {
    // redirect references to index.html to '/'
    PLUG_RULE("/index.html?$", HTTP_GET, plug_redirect, 301, "/"),
    // ensure logging of all other response codes
    PLUG_RULE("", HTTP_ANY, qmc_logging, 0, NULL),
    // rewrite '/' so it will be served from index.html
    PLUG_RULE("/$", HTTP_GET, plug_rewrite_path, 0, "/index.html"),

    // rewrite GET '/logout' to DELETE /api/session
    PLUG_RULE("/logout$", HTTP_GET, plug_rewrite_path, HTTP_DELETE, "/api/session"),

    // redirect '/[a-zA-Z0-9]+' so and direct the browser to the index page (if /logout did not match '|' )
    PLUG_RULE("|/%w+$", HTTP_GET, plug_redirect, 302, "/"),

    // validate a session, but do not enfoce it yet (flag=0) realm="QMC2G API"
    // match /api if followed by a /
    PLUG_RULE("/api%f[/]", HTTP_GET | HTTP_POST | HTTP_PUT | HTTP_DELETE, qmc_authorize, 0, "QMC2G API"),
    // the /api/motd is usable without authentication
    PLUG_RULE("./motd/?$", HTTP_GET, plug_json, 0, json_motd_api),
    // the /api/session is usable without authentication
    PLUG_RULE("./session%f[/%z]/?%f[%d%z]", HTTP_GET | HTTP_POST | HTTP_DELETE, plug_json, 128, json_session_api),
    // validate the presence of an session/authorization for further /api paths (if followed by / )
    PLUG_RULE("/api%f[/]", HTTP_ANY, qmc_check_session, 0, NULL),
    // only one ./ rule will be matched,
    PLUG_RULE("./motors%f[/%z]/?%f[%d%z]", HTTP_GET | HTTP_POST | HTTP_PUT, plug_json, 256, json_motor_api),
    PLUG_RULE("./log$", HTTP_GET, plug_json, 0, json_log_api),
    PLUG_RULE("./time$", HTTP_GET | HTTP_PUT | HTTP_POST, plug_json, 128, json_time_api),
    PLUG_RULE("./reset$", HTTP_POST, plug_json, 0, json_reset_api),
    PLUG_RULE("./settings%f[/%z]/?", HTTP_GET | HTTP_PUT, plug_json, 512, json_settings_api),
    // firmware upload endpoint POST rule
    PLUG_RULE("./firmware$", HTTP_POST, qmc_fw_upload, 0, NULL),
    PLUG_RULE("./system$", HTTP_GET | HTTP_PUT | HTTP_POST, plug_json, 128, json_system_api),
    PLUG_RULE("./users/?$", HTTP_GET, plug_json, 0, json_user_list_api),
    PLUG_RULE("./users/", HTTP_GET | HTTP_PUT | HTTP_POST | HTTP_DELETE, plug_json, 512, json_user_api),

#if PLUG_ZIP_FS_FATFS && DEBUG
    // anything that starts with a / can be served from the webroot.zip
    // try the SD Card first, then the compiled in version. this is mainly for development purposes
    PLUG_RULE("/", HTTP_GET, plug_zip_fs, 0, &webroot_sd),
    // reply with no-content if not found (no sd-card on startup)
    PLUG_RULE("/", HTTP_GET, plug_status, 204, NULL),
#else
    // anything that starts with a / can be served from the webroot.zip
    PLUG_RULE("/", HTTP_GET, plug_zip_fs, 404, &webroot),
#endif
    // unsupported method for everything else
    PLUG_RULE("", HTTP_ANY, plug_status, 405, NULL),
};

/**
 * @brief called within the lwip stack to initialize the webserver
 *
 * @param ctx unused
 */
static void webservice_init_fn(void *ctx)
{
    (void)ctx;
    s_httpd_config.tls     = NULL;
    sss_session_t *session = SE_GetSession();
    sss_key_store_t *ks    = SE_GetKeystore();

    // tls configuration
    if (session != NULL && ks != NULL)
    {
        s_httpd_config.tls = altcp_tls_create_config_server_sss(idWebServerIdCert, idWebServerIdKeyPair, session, ks);
        s_httpd_config.rule_vec = s_qmc_webservice_rules;
        s_httpd_config.rule_cnt = sizeof(s_qmc_webservice_rules) / sizeof(s_qmc_webservice_rules[0]);
        cit_httpd_init(&s_httpd_config);
    }
}

/**
 * @brief  regiser initialization function with lwip
 */
void webservice_init(void)
{
    tcpip_callback(webservice_init_fn, NULL);
}
