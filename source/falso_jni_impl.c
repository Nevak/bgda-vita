#include <falso_jni/FalsoJNI_Impl.h>
#include <falso_jni/FalsoJNI_Logger.h>
#include <string.h>
#include <vitasdk.h>
#include <AFakeNative/keycodes.h>

// System-wide constant that's often used to determine Android version
// https://developer.android.com/reference/android/os/Build.VERSION.html#SDK_INT
// Possible values: https://developer.android.com/reference/android/os/Build.VERSION_CODES
const int SDK_INT = 19; // Android 4.4 / KitKat

/*
 * JNI Methods
*/

NameToMethodID nameToMethodId[] = {
	{ 1, "stringCatcher", METHOD_TYPE_VOID },
	{ 2, "licenseCheck", METHOD_TYPE_VOID }, 
	{ 3, "getLicenseResult", METHOD_TYPE_INT }, 
	{ 4, "expansionIsValid", METHOD_TYPE_BOOLEAN }, 
	{ 5, "getExpansionPath", METHOD_TYPE_OBJECT }, 
	{ 6, "gameHelperReady", METHOD_TYPE_VOID }, 
	{ 7, "playGameBoot", METHOD_TYPE_VOID }, 
	{ 8, "getLocale", METHOD_TYPE_OBJECT }, 
	{ 9, "getTouchScreenNum", METHOD_TYPE_INT }, 
	{ 10, "hasJoyStickMethods", METHOD_TYPE_BOOLEAN }, 
	{ 11, "getApiLevel", METHOD_TYPE_INT }, 
	{ 12, "getDataPath", METHOD_TYPE_OBJECT }, 
	{ 13, "getPadNum", METHOD_TYPE_INT }, 
	{ 14, "openPubData", METHOD_TYPE_VOID },
	{ 15, "closePubData", METHOD_TYPE_VOID },
	{ 16, "timeStampRequest", METHOD_TYPE_VOID },
	{ 17, "hasStartButton", METHOD_TYPE_BOOLEAN },
	{ 18, "getVersionName", METHOD_TYPE_OBJECT },
	{ 19, "confirmFinish", METHOD_TYPE_VOID },
	{ 20, "isJoyStick", METHOD_TYPE_BOOLEAN },
	{ 21, "getButtonList", METHOD_TYPE_INT },
	{ 22, "getConfirmResult", METHOD_TYPE_INT },
	{ 23, "disposeConfirmDialog", METHOD_TYPE_VOID },
	{ 24, "playGameIsSignedIn", METHOD_TYPE_BOOLEAN },
	{ 25, "getPubData", METHOD_TYPE_OBJECT },
	{ 26, "getPubLink", METHOD_TYPE_OBJECT },
	{ 27, "playGameSignInReq", METHOD_TYPE_VOID },
	{ 28, "webToMoreApp", METHOD_TYPE_VOID },
	{ 29, "getSubLibPath", METHOD_TYPE_OBJECT },
	{ 30, "getStore", METHOD_TYPE_INT },
	{ 31, "isPhone", METHOD_TYPE_BOOLEAN },
	{ 32, "getCacheDir", METHOD_TYPE_OBJECT },
	{ 33, "getExternalCacheDir", METHOD_TYPE_OBJECT },
	{ 34, "mountAPKExpansion", METHOD_TYPE_OBJECT },
	{ 35, "getAPKExpansionFileName", METHOD_TYPE_OBJECT },
	{ 36, "mountAPKPatch", METHOD_TYPE_OBJECT },
	{ 37, "getAPKPatchFileName", METHOD_TYPE_OBJECT },
	{ 38, "getAssetPackCount", METHOD_TYPE_INT },
	{ 39, "getAssetPackPath", METHOD_TYPE_OBJECT },
	{ 40, "loadClass", METHOD_TYPE_OBJECT },
	{ 41, "getFilesDir", METHOD_TYPE_OBJECT },
	{ 42, "getCacheDir", METHOD_TYPE_OBJECT },
	{ 43, "isAAB", METHOD_TYPE_BOOLEAN },
	{ 44, "getOrientation", METHOD_TYPE_INT },
	{ 45, "getNaturalOrientation", METHOD_TYPE_INT },
	{ 46, "getDisplayRealWorldSize", METHOD_TYPE_FLOAT },
	{ 47, "getAbsolutePath", METHOD_TYPE_OBJECT },
	{ 48, "getMaxTouchPoints", METHOD_TYPE_INT },
	{ 49, "isPVRTraceActive", METHOD_TYPE_BOOLEAN },
	{ 50, "initCloud", METHOD_TYPE_OBJECT },
};

void stringCatcher(jmethodID id, va_list args) {
	jint arg1 = va_arg(args, jint);
	jstring arg2 = va_arg(args, jstring);
	fjni_logv_info("stringCatcher with %i, %s", arg1, arg2);
}

void dummy(jmethodID id, va_list args) {
	fjni_log_info("dummy");
}

jint getLicenseResult(jmethodID id, va_list args) {
	return 1;
}

jint getTouchScreenNum(jmethodID id, va_list args) {
	return 1;
}

jint getConfirmResult(jmethodID id, va_list args) {
	return 1;
}

jint getStore(jmethodID id, va_list args) {
	return 0;
}

jint getAssetPackCount(jmethodID id, va_list args) {
	return 1;
}

jint getOrientation(jmethodID id, va_list args) {
	return 1;
}

jint getNaturalOrientation(jmethodID id, va_list args) {
	return 1;
}

jfloat getDisplayRealWorldSize(jmethodID id, va_list args) {
	return 1.0f;
}

jint getMaxTouchPoints(jmethodID id, va_list args) {
	return 4;
}

/*
 A = 0x10
 B = 0x20
 X = 0x40
 Y = 0x80
 START = 0x100
 SELECT = 0x200
 HOME = 0x800
 BACK = 0x400
*/

jint getPadNum(jmethodID id, va_list args) {
	return 1;
}

jint getButtonList(jmethodID id, va_list args) {
	jint arg1 = va_arg(args, jint);
	printf("getButtonList(%d)\n", arg1);
	int r = 0;
	r += 0x10; // A
	r += 0x20; // B
	r += 0x40; // X
	r += 0x80; // Y
	r += 0x100; // Start
	r += 0x200; // Select
	return r;
}

jint getApiLevel(jmethodID id, va_list args) {
	return SDK_INT;
}

jboolean expansionIsValid(jmethodID id, va_list args) {
    return JNI_TRUE;
}

jboolean hasStartButton(jmethodID id, va_list args) {
    return JNI_TRUE;
}

jboolean playGameIsSignedIn(jmethodID id, va_list args) {
    return JNI_FALSE;
}

jboolean isPhone(jmethodID id, va_list args) {
	return JNI_FALSE;
}

jboolean isAAB(jmethodID id, va_list args) {
	return JNI_FALSE;
}

jboolean isPVRTraceActive(jmethodID id, va_list args) {
	return JNI_FALSE;
}

jboolean hasJoyStickMethods(jmethodID id, va_list args) {
    return JNI_TRUE;
}

jboolean isJoyStick(jmethodID id, va_list args) {
    return JNI_TRUE;
}

jobject getExpansionPath(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
    char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
    return (jobject)ret;
}

jobject getVersionName(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(4, FIELD_TYPE_BYTE);
    char *arr = ret->array;
	strcpy(arr, "1.0");
    return (jobject)ret;
}

jobject getDataPath(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
    char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
    return (jobject)ret;
}

jobject getPubData(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("Port by Rinnegatamante") + 1, FIELD_TYPE_BYTE);
    char *arr = ret->array;
	strcpy(arr, "Port by Rinnegatamante");
    return (jobject)ret;
}

jobject getPubLink(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("https://vitadb.rinnegatamante.it") + 1, FIELD_TYPE_BYTE);
    char *arr = ret->array;
	strcpy(arr, "https://vitadb.rinnegatamante.it");
    return (jobject)ret;
}

jobject getLocale(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(2, FIELD_TYPE_BYTE);
    char *arr = ret->array;
	
	int res;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &res);
	switch (res) {
	case SCE_SYSTEM_PARAM_LANG_JAPANESE:
		strcpy(arr, "ja");
		break;
	case SCE_SYSTEM_PARAM_LANG_SPANISH:
		strcpy(arr, "es");
		break;
	case SCE_SYSTEM_PARAM_LANG_FRENCH:
		strcpy(arr, "fr");
		break;
	case SCE_SYSTEM_PARAM_LANG_GERMAN:
		strcpy(arr, "de");
		break;
	default:
		strcpy(arr, "en");
		break;
	}
	
    return (jobject)ret;
}

jobject getSubLibPath(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}

jobject getCacheDir(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}

jobject getAbsolutePath(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}

jobject getExternalCacheDir(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}

jobject mountAPKExpansion(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}

jobject getAPKExpansionFileName(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "bgda");
	return (jobject)ret;
}

jobject mountAPKPatch(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}

jobject getAPKPatchFileName(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "bgda");
	return (jobject)ret;
}

jobject getAssetPackPath(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}

jobject loadClass(jmethodID id, va_list args) {
	jstring className = va_arg(args, jstring);
	
	fjni_logv_err("loadClass with %s", className);
	
	JavaDynArray * ret = jda_alloc(strlen("com.rinnegatamante.bgda.BGDA") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "com.rinnegatamante.bgda.BGDA");
	return (jobject)ret;
}

jobject getFilesDir(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	strcpy(arr, "ux0:data/bgda");
	return (jobject)ret;
}


/*
public abstract class Cloud {
    protected static native void readback(byte[] bArr, long j);

    public abstract boolean isAvailable();

    public abstract void update();

    public abstract void write(byte[] bArr);

    public static Cloud init(String filename) {
        if (!Util.getMetaData(Activity.Get()).getBoolean("com.jbe.cloud") || Activity.Get().getStore() != Activity.Store.GOOGLEPLAY.ordinal()) {
            return null;
        }
        return new GooglePlay(filename);
    }
}
*/

jobject initCloud(jmethodID id, va_list args) {
	// log
	fjni_log_err("initCloud");
	//jstring arg2 = va_arg(args, jstring);
	//fjni_logv_err("initCloud with %s", arg2);
    // Create a minimal, non-null "object" to keep the code happy.
    // For example, a 1-byte array or a small "dummy" object:
   // JavaDynArray * dummy = jda_alloc(1, FIELD_TYPE_OBJECT);
    // or if you have a jdo_alloc for objects, do that instead.
    // Just ensure it's not NULL and won't break if the code calls methods on it.
    
    return NULL;
}

MethodsBoolean methodsBoolean[] = {
	{ 4, expansionIsValid }, 
	{ 10, hasJoyStickMethods }, 
	{ 17, hasStartButton },
	{ 20, isJoyStick },
	{ 24, playGameIsSignedIn },
	{ 31, isPhone },
	{ 43, isAAB },
	{ 49, isPVRTraceActive },
};

MethodsByte methodsByte[] = {};
MethodsChar methodsChar[] = {};
MethodsDouble methodsDouble[] = {};
MethodsFloat methodsFloat[] = {
	{ 46, getDisplayRealWorldSize }
};
MethodsLong methodsLong[] = {};
MethodsShort methodsShort[] = {};

MethodsInt methodsInt[] = {
	{ 3, getLicenseResult }, 
	{ 9, getTouchScreenNum },
	{ 11, getApiLevel },
	{ 13, getPadNum },
	{ 21, getButtonList },
	{ 22, getConfirmResult },
	{ 30, getStore },
	{ 38, getAssetPackCount },
	{ 44, getOrientation },
	{ 45, getNaturalOrientation },
	{ 48, getMaxTouchPoints },
};

MethodsObject methodsObject[] = {
	{ 5, getExpansionPath }, 
	{ 8, getLocale },
	{ 12, getDataPath },
	{ 18, getVersionName },
	{ 25, getPubData },
	{ 26, getPubLink },
	{ 29, getSubLibPath },
	{ 32, getCacheDir },
	{ 33, getExternalCacheDir },
	{ 34, mountAPKExpansion },
	{ 35, getAPKExpansionFileName },
	{ 36, mountAPKPatch },
	{ 37, getAPKPatchFileName },
	{ 39, getAssetPackPath },
	{ 40, loadClass },
	{ 41, getFilesDir },
	{ 42, getCacheDir },
	{ 47, getAbsolutePath },
	{ 50, initCloud },
};

MethodsVoid methodsVoid[] = {
	{ 1, stringCatcher }, 
	{ 2, dummy }, // licenseCheck
	{ 6, dummy }, // gameHelperReady
	{ 7, dummy }, // playGameBoot
	{ 14, dummy }, // openPubData
	{ 15, dummy }, // closePubData
	{ 16, dummy }, // timeStampRequest
	{ 19, dummy }, // confirmFinish
	{ 23, dummy }, // disposeConfirmDialog
	{ 27, dummy }, // playGameSignInReq
	{ 28, dummy }, // webToMoreApp
};

/*
 * JNI Fields
*/

// System-wide constant that applications sometimes request
// https://developer.android.com/reference/android/content/Context.html#WINDOW_SERVICE
char WINDOW_SERVICE[] = "window";

NameToFieldID nameToFieldId[] = {
	{ 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT }, 
	{ 1, "SDK_INT", FIELD_TYPE_INT },
};

FieldsBoolean fieldsBoolean[] = {};
FieldsByte fieldsByte[] = {};
FieldsChar fieldsChar[] = {};
FieldsDouble fieldsDouble[] = {};
FieldsFloat fieldsFloat[] = {};
FieldsInt fieldsInt[] = {
	{ 1, SDK_INT },
};
FieldsObject fieldsObject[] = {
	{ 0, WINDOW_SERVICE },
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};

__FALSOJNI_IMPL_CONTAINER_SIZES
