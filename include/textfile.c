#define LINE_LENGTH 75

wchar_t* trimWhitespace(wchar_t* str)
{
  wchar_t* end;

  while(isspace(*str)) 
    str++;

  if(*str == 0)
    return str;

  end = str + wcslen(str) - 1;

  while(end > str && isspace(*end)) 
    end--;

  *(end + 1) = 0;

  return str;
}

int readTextFile(wchar_t** lines) {

    //get absolute path in case of autostart
    wchar_t txtPath[MAX_PATH];
    GetModuleFileName(NULL, txtPath, ARRAY_SIZE(txtPath));
    PathRemoveFileSpec(txtPath);
    wcscat(txtPath, L"\\disabledApps.txt");
    FILE *fp = _wfopen(txtPath, L"r");

    int i = 0;

    if (fp == 0)
        return i;

    while(!feof(stdin)) {
      lines[i] = malloc(LINE_LENGTH * sizeof(wchar_t*));
      if (fgetws(lines[i], LINE_LENGTH, fp) == NULL) {
        free(lines[i]);
        break;
      }

      trimWhitespace(lines[i]);
      i++;
    }

    fclose(fp);
    return i;
}