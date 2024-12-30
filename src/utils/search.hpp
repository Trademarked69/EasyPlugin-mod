#pragma once

#include <string>
#include <psp2/ime_dialog.h>
#include "json.hpp"

#define IME_DIALOG_RESULT_NONE 0
#define IME_DIALOG_RESULT_RUNNING 1
#define IME_DIALOG_RESULT_FINISHED 2
#define IME_DIALOG_RESULT_CANCELED 3

using json = nlohmann::json;
using namespace std;

int initImeDialog(char *title, char *initial_text, int max_text_length);
uint16_t *getImeDialogInputTextUTF16();
uint8_t *getImeDialogInputTextUTF8();
int isImeDialogRunning();
int updateImeDialog();

json sortJson(string filter, json original);

string toLowercase(string strToConvert);