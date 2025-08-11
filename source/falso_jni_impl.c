#include <falso_jni/FalsoJNI.h>
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
	{ 51, "getISO3Language", METHOD_TYPE_OBJECT },
	{ 52, "initSocial", METHOD_TYPE_OBJECT },
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
	// Got this number by analyzing the decompiled Java code
	return 4;
}

jint getNaturalOrientation(jmethodID id, va_list args) {
	// Got this number by analyzing the decompiled Java code
	return 2;
}

jfloat getDisplayRealWorldSize(jmethodID id, va_list args) {
	// The java code does:
	
    // public float getDisplayRealWorldSize() {
    //     DisplayMetrics dm = new DisplayMetrics();
    //     this.mDisplay.getMetrics(dm);
    //     return (float) Math.sqrt(Math.pow((double) (((float) dm.widthPixels) / dm.xdpi), 2.0d) + Math.pow((double) (((float) dm.heightPixels) / dm.ydpi), 2.0d));
    // }

	// Seems like the screen size in inches. The vita screen is 5 inches.
	return 5.0f;
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
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject getVersionName(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "1.0");
}

jobject getDataPath(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject getPubData(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "Port by Nevak");
}

jobject getPubLink(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "https://vitadb.rinnegatamante.it");
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
	
    //return (jobject)ret;
	return jni->NewStringUTF(&jni, arr);
}

jobject getISO3Language(jmethodID id, va_list args) {
	JavaDynArray * ret = jda_alloc(3, FIELD_TYPE_BYTE);
	char *arr = ret->array;
	
	
	int res;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &res);
	
	printf("getISO3Language(%d)\n", res);

	switch (res) {
		case SCE_SYSTEM_PARAM_LANG_JAPANESE:
			strcpy(arr, "jpn");
			break;
		case SCE_SYSTEM_PARAM_LANG_SPANISH:
			strcpy(arr, "spa");
			break;
		case SCE_SYSTEM_PARAM_LANG_FRENCH:
			strcpy(arr, "fra");
			break;
		case SCE_SYSTEM_PARAM_LANG_GERMAN:
			strcpy(arr, "deu");
			break;
		default:
			printf("default\n");
			strcpy(arr, "eng");
			break;
	}
	
	printf("getISO3Language() %s\n", arr);

	//return (jobject)arr;
	return jni->NewStringUTF(&jni, arr);
}

jobject getSubLibPath(jmethodID id, va_list args) {
    return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject getCacheDir(jmethodID id, va_list args) {
	// JavaDynArray * ret = jda_alloc(strlen("ux0:data/bgda") + 1, FIELD_TYPE_BYTE);
	// char *arr = ret->array;
	// strcpy(arr, "ux0:data/bgda");
	// return (jobject)ret;
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject getAbsolutePath(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject getExternalCacheDir(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject mountAPKExpansion(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject getAPKExpansionFileName(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "bgda");
}

jobject mountAPKPatch(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
}

jobject getAPKPatchFileName(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "bgda");
}

jobject getAssetPackPath(jmethodID id, va_list args) {
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
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
	return jni->NewStringUTF(&jni, "ux0:data/bgda");
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
	//fjni_log_err("initCloud");
	//jstring arg2 = va_arg(args, jstring);
	//fjni_logv_err("initCloud with %s", arg2);
    // Create a minimal, non-null "object" to keep the code happy.
    // For example, a 1-byte array or a small "dummy" object:
   // JavaDynArray * dummy = jda_alloc(1, FIELD_TYPE_OBJECT);
    // or if you have a jdo_alloc for objects, do that instead.
    // Just ensure it's not NULL and won't break if the code calls methods on it.
    
    return NULL;
}

jobject initSocial(jmethodID id, va_list args) {
	fjni_log_err("initSocial CALLED");
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
	{ 51, getISO3Language },
	{ 52, initSocial },
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

jstring str_window_service;
jstring str_release;
jstring str_device;
jstring str_model;
jstring str_cpu_abi;

void init_jni_fields(JNIEnv *env) {
	str_window_service = (*env)->NewStringUTF(env, WINDOW_SERVICE);
	str_release = (*env)->NewStringUTF(env, "1.0.7");
	str_device = (*env)->NewStringUTF(env, "vita");
	str_model = (*env)->NewStringUTF(env, "PlayStation Vita");
	str_cpu_abi = (*env)->NewStringUTF(env, "armeabi-v7a");
	
	// Populate the fieldsObject array after strings are created
	fieldsObject[0].value = str_window_service;
	fieldsObject[1].value = str_release;
	fieldsObject[2].value = str_device;
	fieldsObject[3].value = str_model;
	fieldsObject[4].value = str_cpu_abi;
}

NameToFieldID nameToFieldId[] = {
	{ 0, "WINDOW_SERVICE", FIELD_TYPE_OBJECT }, 
	{ 1, "SDK_INT", FIELD_TYPE_INT },
	{ 2, "RELEASE", FIELD_TYPE_OBJECT },
	{ 3, "DEVICE", FIELD_TYPE_OBJECT },
	{ 4, "MODEL", FIELD_TYPE_OBJECT },
	{ 5, "CPU_ABI", FIELD_TYPE_OBJECT },
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
	{ 0, NULL },
	{ 2, NULL }, // RELEASE
	{ 3, NULL }, // DEVICE
	{ 4, NULL }, // MODEL
	{ 5, NULL }, // CPU_ABI
};
FieldsLong fieldsLong[] = {};
FieldsShort fieldsShort[] = {};


__FALSOJNI_IMPL_CONTAINER_SIZES
