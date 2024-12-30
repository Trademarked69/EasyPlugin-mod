#include <vita2d.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h> 
#include <psp2/io/fcntl.h>
#include <psp2/sqlite.h>
#include <psp2/apputil.h>
#include <psp2/display.h>
#include <psp2/ime_dialog.h>
#include <string>

#include "main.hpp"
#include "sqlite3.h"
#include "net/download.hpp"
#include "utils/filesystem.hpp"
#include "utils/search.hpp"
#include "utils/format.hpp"

#include "screens/list.hpp"
#include "screens/details.hpp"
#include "screens/popup.hpp"

SceCtrlData pad;

AppInfo::AppInfo(string p_appID, string p_title, string fileLocation) {
    appID = p_appID;
    title = p_title;
    icon = fileLocation.find(".dds") == string::npos ? vita2d_load_PNG_file(fileLocation.c_str()) : NULL;
}

static int callback(void *data, int argc, char **argv, char **column_name) {
    vector<AppInfo> *ret = (vector<AppInfo>*) data;

	ret->emplace_back(string(argv[0]), string(argv[1]), string(argv[2]));
	return 0;
}

void initSceAppUtil()
{
	// Init SceAppUtil
	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);

	// Set common dialog config
	SceCommonDialogConfigParam config;
	sceCommonDialogConfigParamInit(&config);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&config.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&config.enterButtonAssign);
	sceCommonDialogSetConfigParam(&config);
}

int getAppData(vector<AppInfo> &ret) {
    SceIoDirent dirInfo;
    SceUID folder;

	ret.emplace_back("ALL", "All", "");
	ret.emplace_back("main", "Livearea (main)", "");

    sqlite3 *db = NULL;

    sceSysmoduleLoadModule(SCE_SYSMODULE_SQLITE);

    sqlite3_rw_init();

    int rc = sqlite3_open_v2("ur0:shell/db/app.db", &db, SQLITE_OPEN_READONLY, NULL);
    if(rc != SQLITE_OK) {
        return -1;
    }

    sqlite3_exec(db, "SELECT titleId, title, iconPath FROM tbl_appinfo_icon WHERE NOT titleId=\"NULL\" ORDER BY title ASC", callback, &ret, NULL);

	if (db != NULL)
		sqlite3_close(db);
	sqlite3_rw_exit();

    sceSysmoduleUnloadModule(SCE_SYSMODULE_SQLITE);

    return 0;
}

int main() {
    vita2d_init();
    initSceAppUtil();
    
    Filesystem::removePath("ux0:data/Easy_Plugins");
    Filesystem::mkDir("ux0:data/Easy_Plugins");

    vita2d_set_clear_color(RGBA8(255,255,255,255));

    vita2d_texture *bgIMG = vita2d_load_PNG_file("ux0:app/ESPL00009/resources/bg.png");
    
    httpInit();
    netInit();
    curlDownload("http://rinnegatamante.it/vitadb/list_plugins_json.php", "ux0:data/Easy_Plugins/plugins.json");

    SharedData sharedData;

    if(doesDirExist("ux0:tai")) sharedData.taiConfigPath = "ux0:tai/";
    else if(doesDirExist("ur0:tai")) sharedData.taiConfigPath = "ur0:tai/";
    else return -1;

    sharedData.taiConfig = Filesystem::readFile(sharedData.taiConfigPath+"config.txt");

    sharedData.plugins = json::parse(Filesystem::readFile("ux0:data/Easy_Plugins/plugins.json"));

    getAppData(sharedData.appData);

    sharedData.original = sharedData.plugins;

    List listView;
    Popup popupView;
    Details detailsView;

    while(1) {
        sceCtrlPeekBufferPositive(0, &pad, 1);
        vita2d_start_drawing();
        vita2d_clear_screen();

        vita2d_draw_texture(bgIMG, 0, 0);

        if(pad.buttons != SCE_CTRL_CROSS) sharedData.blockCross = false;
        if(pad.buttons != SCE_CTRL_CIRCLE) sharedData.blockCircle = false;
        if(pad.buttons != SCE_CTRL_START) sharedData.blockStart = false;
        
        if(sharedData.scene == 0) listView.draw(sharedData, pad.buttons);
        
        if(sharedData.scene == 1) detailsView.draw(sharedData, pad.buttons);

        if(sharedData.scene == 2) popupView.draw(sharedData, pad.buttons);

        vita2d_end_drawing();
        vita2d_common_dialog_update();
        vita2d_swap_buffers();
        sceDisplayWaitVblankStart();

        if(pad.buttons == SCE_CTRL_SELECT) {
            break;
        }
        if(pad.buttons == SCE_CTRL_START && !sharedData.blockStart) {
            if(sharedData.scene != 2) scePowerRequestColdReset();
        }
    }

    httpTerm();
    netTerm();
    vita2d_free_font(sharedData.font);
    vita2d_free_texture(bgIMG);
    for(int i=0; i<sharedData.appData.size();i++) {
        if(sharedData.appData[i].icon != NULL)
        vita2d_free_texture(sharedData.appData[i].icon);
    }
    for(int i=0;i<sharedData.screenshots.size();i++) {
        if(sharedData.screenshots[i] != NULL)
        vita2d_free_texture(sharedData.screenshots[i]);
    }
    listView.free();
    detailsView.free();
    popupView.free();
    vita2d_fini();
    
    sceKernelExitProcess(0);
    return 0;
}
