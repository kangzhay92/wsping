#pragma once

extern vp_app* vp_main(int argc, char** argv);

#if (defined(VP_GUI) && defined(_WIN32))
static char** _parse_argv(LPWSTR wcmd, int* oargc)
{
	int argc = 0;
	char** ret = NULL;
	char* args;

	LPWSTR* wargv = CommandLineToArgvW(wcmd, &argc);
	if (!wargv) {
		return NULL;
	} else {
		size_t sz = wcslen(wcmd) * 4;
		ret  = (char**)calloc(1, (argc + 1) * sizeof(char*) + sz);
		args = (char*)&ret[argc + 1];
		int n;
		for (int i = 0; i < argc; ++i) {
			n = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, args, (int)sz, NULL, NULL);
			if (n == 0) {
				return NULL;
			}
			ret[i] = args;
			sz -= n;
			args += n;
		}
		LocalFree(wargv);
	}

	*oargc = argc;
	return ret;
}
int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int ncmd)
{
	int argc = 0;
	char** argv = _parse_argv(GetCommandLineW(), &argc);
	if (!argv) {
		return 1;
	}
	vp_app* app = vp_main(argc, argv);
#ifdef VP_C_INTERFACE
	vp_app_run(app);
	vp_app_shutdown(app);
#else
	app->run();
	app->shutdown();
#endif
	return 0;
}
#else 
int main(int argc, char** argv)
{
	vp_app* app = vp_main(argc, argv);
#ifdef VP_C_INTERFACE
	vp_app_run(app);
	vp_app_shutdown(app);
#else
	app->run();
	app->shutdown();
#endif
	return 0;
}
#endif
