
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>

typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;

static uint8 *objectScriptData;
static int objectScriptOffset;
static int objectScriptSize;

static int getFileSize(FILE *fp) {
	int pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	int sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return sz;
}

static int16 readNextScriptInt() {
	assert(objectScriptOffset < objectScriptSize);
	uint16 num = objectScriptData[objectScriptOffset] | (objectScriptData[objectScriptOffset + 1] << 8);
	objectScriptOffset += 2;
	return (int16)num;
}

static const char *readNextScriptObjectName() {
	const char *name = "";
	int len = readNextScriptInt();
	assert(len >= 0);
	if (len == 0) {
		name = "<CURRENT_OBJECT>";
	} else {
		name = (const char *)(objectScriptData + objectScriptOffset);
	}
	objectScriptOffset += len;
	return name;
}

static const char *readNextScriptEval(const char *dst) {
	static char evalBuffer[256];
	int op = readNextScriptInt();
	int val = readNextScriptInt();
	switch (op) {
	case 0:
		snprintf(evalBuffer, sizeof(evalBuffer), "%s = %d", dst, val);
		break;
	case 1:
		snprintf(evalBuffer, sizeof(evalBuffer), "%s += %d", dst, val);
		break;
	case 2:
		snprintf(evalBuffer, sizeof(evalBuffer), "%s -= %d", dst, val);
		break;
	case 3:
		snprintf(evalBuffer, sizeof(evalBuffer), "%s *= %d", dst, val);
		break;
	case 4:
		snprintf(evalBuffer, sizeof(evalBuffer), "%s /= %d", dst, val);
		break;
	default:
		printf("Invalid eval operator %d", op);
		exit(0);
	}
	return evalBuffer;
}

static const char *readNextScriptTest(const char *src) {
	static char testBuffer[256];
	int op = readNextScriptInt();
	if (op == -1) {
		char orTestBuffer[64];
		int num = readNextScriptInt();
		for (int i = 0; i < num; ++i) {
			int cmp1 = readNextScriptInt();
			int cmp2 = readNextScriptInt();
			assert(cmp1 <= cmp2);
			snprintf(orTestBuffer,sizeof(orTestBuffer), "%d <= %s <= %d", cmp1, src, cmp2);
			if (i == 0) {
				strcpy(testBuffer, orTestBuffer);
			} else {
				strcat(testBuffer, " || ");
				strcat(testBuffer, orTestBuffer);
			}
		}
	} else {
		int cmp = readNextScriptInt();
		switch (op) {
		case 0:
			snprintf(testBuffer, sizeof(testBuffer), "%d == %s", cmp, src);
			break;
		case 1:
			snprintf(testBuffer, sizeof(testBuffer), "%d != %s", cmp, src);
			break;
		case 2:
			snprintf(testBuffer, sizeof(testBuffer), "%d > %s", cmp, src);
			break;
		case 3:
			snprintf(testBuffer, sizeof(testBuffer), "%d < %s", cmp, src);
			break;
		case 4:
			snprintf(testBuffer, sizeof(testBuffer), "%d >= %s", cmp, src);
			break;
		case 5:
			snprintf(testBuffer, sizeof(testBuffer), "%d <= %s", cmp, src);
			break;
		default:
			printf("Invalid test operator %d\n", op);
			exit(0);
		}
	}
	return testBuffer;
}

static void getScaledCoord(int *mul, int *div, int *d) {
	*mul = readNextScriptInt();
	*div = readNextScriptInt();
	*d = readNextScriptInt();
}

static const char *getKeyIdentifier(int key) {
	static char keyId[20];
	switch (key) {
	case 13:
		strcpy(keyId, "VK_ENTER");
		break;
	case 16:
		strcpy(keyId, "VK_SHIFT");
		break;
	case 32:
		strcpy(keyId, "VK_SPACE");
		break;
	case 37:
		strcpy(keyId, "VK_LEFT");
		break;
	case 38:
		strcpy(keyId, "VK_UP");
		break;
	case 39:
		strcpy(keyId, "VK_RIGHT");
		break;
	case 40:
		strcpy(keyId, "VK_DOWN");
		break;
	default:
		snprintf(keyId, sizeof(keyId), "%d", key);
		break;
	}
	return keyId;
}

static void decodeObjectScript() {
	int statementCount = 0;
	objectScriptOffset = 0;
	while (objectScriptOffset < objectScriptSize) {
		int endOffset = readNextScriptInt();
		int op;

		// conditions
//		printf("if ( // statement %d, offset 0x%04X/0x%04X\n", statementCount, objectScriptOffset, objectScriptSize);
		printf("SCRIPT %d OFFSET 0x%X\n", statementCount, objectScriptOffset);
		printf("\tCONDITIONS SEQUENCE\n");
		while ((op = readNextScriptInt()) != 0) {
			printf("\t\t");
			switch (op) {
			case 10: {
					printf("TRUE");
				}
				break;
			case 100: {
					int rnd = readNextScriptInt();
					printf("IS_IN_RANDOM_RANGE(rnd=%d)", rnd);
				}
				break;
			case 500: {
					int key = readNextScriptInt();
					printf("IS_KEY_PRESSED(key=%s)", getKeyIdentifier(key));
				}
				break;
			case 510: {
					int key = readNextScriptInt();
					printf("IS_KEY_NOT_PRESSED(key=%s)", getKeyIdentifier(key));
				}
				break;
			case 3000: {
					const char *name = readNextScriptObjectName();
					int state = readNextScriptInt();
					printf("object['%s'].state == %d", name, state);
				}
				break;
			case 3050: {
					const char *name = readNextScriptObjectName();
					int x1 = readNextScriptInt();
					int y1 = readNextScriptInt();
					int x2 = readNextScriptInt();
					int y2 = readNextScriptInt();
					printf("IS_OBJECT_IN_RECT(object['%s'], x1=%d, y1=%d, x2=%d, y2=%d)", name, x1, y1, x2, y2);
				}
				break;
			case 3100: {
					const char *name = readNextScriptObjectName();
					objectScriptOffset += 14; // XXX
					printf("cop_testPrevObjectTransformXPos(object['%s'])", name);
				}
				break;
			case 3105: {
					const char *name = readNextScriptObjectName();
					objectScriptOffset += 14; // XXX
					printf("IS_OBJECT_X_IN_AREA(object['%s'])", name);
				}
				break;
			case 3110: {
					const char *name = readNextScriptObjectName();
					objectScriptOffset += 14; // XXX
					printf("IS_OBJECT_X_IN_AREA(object['%s'])", name);
				}
				break;
			case 3300: {
					const char *name = readNextScriptObjectName();
					int value = readNextScriptInt();
					printf("object['%s'].unk1A == %d", name, value);
				}
				break;
			case 3310: {
					const char *name = readNextScriptObjectName();
					int value = readNextScriptInt();
					printf("object['%s'].unk18 == %d", name, value);
				}
				break;
			case 3400: {
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "GET_FIRST_ANIM_FRAME(object['%s'])", name);
					const char *test = readNextScriptTest(data);
					printf("%s", test);
				}
				break;
			case 3500: {
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "GET_CURRENT_ANIM_FRAME(object['%s'])", name);
					const char *test = readNextScriptTest(data);
					printf("%s", test);
				}
				break;
			case 3510: {
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "GET_PREVIOUS_ANIM_FRAME(object['%s'])", name);
					const char *test = readNextScriptTest(data);
					printf("%s", test);
				}
				break;
			case 3600: {
					int var = readNextScriptInt();
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "object['%s'].varsTable[%d]", name, var);
					const char *test = readNextScriptTest(data);
					printf("%s", test);
				}
				break;
			case 3700: {
					const char *name1 = readNextScriptObjectName();
					objectScriptOffset += 12; // XXX
					const char *name2 = readNextScriptObjectName();
					objectScriptOffset += 12; // XXX
					printf("UNK_3700");
				}
				break;
			case 3710: {
					readNextScriptObjectName();
					objectScriptOffset += 12; // XXX
					readNextScriptObjectName(); // XXX
					objectScriptOffset += 12; // XXX
					printf("UNK_3710");
				}
				break;
			case 6000: {
					int var = readNextScriptInt();
					char data[100];
					snprintf(data, sizeof(data), "varsTable[%d]", var);
					const char *test = readNextScriptTest(data);
					printf("%s", test);
				}
				break;
			case 6500: {
					int num = readNextScriptInt();
					printf("IS_CURRENT_BAG_ACTION == %d", num);
				}
				break;
			case 7000: {
					int box = readNextScriptInt();
					const char *name = readNextScriptObjectName();
					objectScriptOffset += 6 * 4;
					printf("IS_IN_BOX(box=%d, object['%s'])", box, name);
				}
				break;
			case 7500: {
					int box = readNextScriptInt();
					const char *name = readNextScriptObjectName();
					objectScriptOffset += 6 * 4;
					printf("IS_IN_BOX_2(box=%d, object['%s'])", box, name);
				}
				break;
			case 10000: {
					int len = readNextScriptInt();
					const char *name = (const char *)(objectScriptData + objectScriptOffset);
					objectScriptOffset += len;
					printf("IS_CURRENT_BAG_OBJECT == %s", name);
				}
				break;
			case 25000: {
					const char *test = readNextScriptTest("_lastDialogueEndedId");
					printf("%s", test);
				}
				break;
			case 30000: {
					int num = readNextScriptInt();
					printf("IS_NEXT_SCENE(scene=%d)", num);
				}
				break;
			default:
				printf("Unhandled condition %d\n", op);
				exit(0);
			}
			printf("\n");
		}

		// operators
		printf("\tOPERATORS SEQUENCE\n");
		while (objectScriptOffset < endOffset) {
			op = readNextScriptInt();
			printf("\t\t");
			switch (op) {
			case 100: {
					printf("END_OF_OBJECT_SCRIPT");
				}
				break;
			case 3000: {
					const char *name = readNextScriptObjectName();
					int val = readNextScriptInt();
					printf("INIT_OBJECT(object['%s'], mode=%d)", name, val);
				}
				break;
			case 3100: {
					objectScriptOffset += 6;
					const char *eval = readNextScriptEval("CURRENT_OBJECT.xPosCur2");
					printf("%s", eval);
				}
				break;
			case 3110: {
					objectScriptOffset += 6;
					const char *eval = readNextScriptEval("CURRENT_OBJECT.yPosCur2");
					printf("%s", eval);
				}
				break;
			case 3300: {
					const char *name = readNextScriptObjectName();
					int val = readNextScriptInt();
					printf("object['%s'].unk18 = %d", name, val);
				}
				break;
			case 3400: {
					const char *name = readNextScriptObjectName();
					int a0 = readNextScriptInt();
					int a2 = readNextScriptInt();
					printf("ADJUST_POS(object['%s'], %d, %d, 0, 0, 0, 0)", name, a2, a0);
				}
				break;
			case 3430: {
					const char *name = readNextScriptObjectName();
					int a0 = readNextScriptInt();
					int a2 = readNextScriptInt();
					int a4 = readNextScriptInt();
					int a6 = readNextScriptInt();
					printf("ADJUST_POS(object['%s'], %d, %d, 1, %d, 1, %d)", name, a2, a0, a4, a6);
				}
				break;
			case 3440: {
					const char *name = readNextScriptObjectName();
					printf("SETUP_OBJECT_POS(object['%s'], 1, 2, 1)", name);
					objectScriptOffset += 16;
				}
				break;
			case 3480: {
					const char *name = readNextScriptObjectName();
					printf("SETUP_OBJECT_POS(object['%s'], 1, 2, 3)", name);
					objectScriptOffset += 20;
				}
				break;
			case 3500: {
					const char *name = readNextScriptObjectName();
					int a0 = readNextScriptInt();
					printf("ADJUST_POS(object['%s'], 1, %d, 0, 0, 0, 0)", name, a0);
				}
				break;
			case 3530: {
					const char *name = readNextScriptObjectName();
					int a0 = readNextScriptInt();
					int a2 = readNextScriptInt();
					int a4 = readNextScriptInt();
					printf("ADJUST_POS(object['%s'], 1, %d, 1, %d, 1, %d)", name, a0, a2, a4);
				}
				break;
			case 3540: {
					const char *name = readNextScriptObjectName();
					printf("SETUP_OBJECT_POS(object['%s'], 0, 2, 1)", name);
					objectScriptOffset += 14;
				}
				break;
			case 3560: {
					const char *name = readNextScriptObjectName();
					printf("SETUP_OBJECT_POS(object['%s'], 0, 2, 2)", name);
					objectScriptOffset += 26;
				}
				break;
			case 4000: {
					int var = readNextScriptInt();
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "object['%s'].varsTable[%d]", name, var);
					const char *eval = readNextScriptEval(data);
					printf("%s", eval);
				}
				break;
			case 4100: {
					const char *name = readNextScriptObjectName();
					int var1 = readNextScriptInt();
					int var2 = readNextScriptInt();
					int var3 = readNextScriptInt();
					int var4 = readNextScriptInt();
					printf("TRANSLATE_X_POS(object['%s'], %d, %d, %d, %d)", name, var1, var2, var3, var4);
				}
				break;
			case 4200: {
					const char *name = readNextScriptObjectName();
					int var1 = readNextScriptInt();
					int var2 = readNextScriptInt();
					int var3 = readNextScriptInt();
					int var4 = readNextScriptInt();
					printf("TRANSLATE_Y_POS(object['%s'], %d, %d, %d, %d)", name, var1, var2, var3, var4);
				}
				break;
			case 5000: {
					const char *name = readNextScriptObjectName();
					int mode = readNextScriptInt();
					printf("object['%s'].mode = %d", name, mode);
					if (mode == 2) {
						int rnd = readNextScriptInt();
						printf(" rnd = %d", rnd);
					}
				}
				break;
			case 5100: {
					const char *name = readNextScriptObjectName();
					int x = readNextScriptInt();
					int y = readNextScriptInt();
					printf("object['%s'] { .xOrigin = %d, .yOrigin = %d }", name, x, y);
				}
				break;
			case 5110: {
					const char *name = readNextScriptObjectName();
					objectScriptOffset += 12; // XXX
					printf("object['%s'].transform { .xOrigin, .yOrigin }", name);
				}
				break;
			case 5112: {
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "object['%s'].xOrigin", name);
					const char *eval = readNextScriptEval(data);
					printf("%s", eval);
				}
				break;
			case 5114: {
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "object['%s'].yOrigin", name);
					const char *eval = readNextScriptEval(data);
					printf("%s", eval);
				}
				break;
			case 5500: {
					const char *name = readNextScriptObjectName();
					int val = readNextScriptInt();
					printf("object['%s'].motionNum = %d", name, val);
				}
				break;
			case 6000: {
					int var = readNextScriptInt();
					char data[100];
					snprintf(data, sizeof(data), "varsTable[%d]", var);
					const char *eval = readNextScriptEval(data);
					printf("%s", eval);
				}
				break;
			case 6100: {
					int var = readNextScriptInt();
					printf("varsTable[%d] = SCENE_NUMBER", var);
				}
				break;
			case 7000: {
					int box = readNextScriptInt();
					int num = readNextScriptInt();
					printf("Box[%d][%d].disable", box, num);
				}
				break;
			case 7010: {
					int box = readNextScriptInt();
					int num = readNextScriptInt();
					printf("Box[%d][%d].enable", box, num);
				}
				break;
			case 7100: {
					const char *eval = readNextScriptEval("BOXES.X1,BOXES.X2");
					printf("%s", eval);
				}
				break;
			case 7110: {
					const char *eval = readNextScriptEval("BOXES.Y1,BOXES.Y2");
					printf("%s", eval);
				}
				break;
			case 7200: {
					int box = readNextScriptInt();
					int num = readNextScriptInt();
					const char *name = readNextScriptObjectName();
					char data[100];
					snprintf(data, sizeof(data), "object['%s']", name);
					int x1mul, x1div, x1d;
					getScaledCoord(&x1mul, &x1div, &x1d);
					int x2mul, x2div, x2d;
					getScaledCoord(&x2mul, &x2div, &x2d);
					int y1mul, y1div, y1d;
					getScaledCoord(&y1mul, &y1div, &y1d);
					int y2mul, y2div, y2d;
					getScaledCoord(&y2mul, &y2div, &y2d);
					printf("set_box_from_object ( [%d][%d], object['%s'], sx1=%d,%d,%d, sy1=%d,%d,%d, sx2=%d,%d,%d, sy2=%d,%d,%d )", box, num, name, x1mul, x1div, x1d, y1mul, y1div, y1d, x2mul, x2div, x2d, y2mul, y2div, y2d);
				}
				break;
			case 7300: {
					int x1 = readNextScriptInt();
					int y1 = readNextScriptInt();
					int x2 = readNextScriptInt();
					int y2 = readNextScriptInt();
					printf("EXTEND_BOXES(x1=%d, y1=%d, x2=%d, y2=%d)", x1, y1, x2, y2);
				}
				break;
			case 8000: {
					int index = readNextScriptInt();
					printf("save_object_status ( index=%d )", index);
				}
				break;
			case 20000: {
					int num = readNextScriptInt();
					int priority = readNextScriptInt();
					printf("PLAY_SOUND_LOWER_EQUAL_PRIORITY(num=%d, priority=%d)", num, priority);
				}
				break;
			case 25000: {
					const char *str[4];
					for (int i = 0; i < 4; ++i) {
						int len = readNextScriptInt();
						str[i] = (const char *)(objectScriptData + objectScriptOffset);
						objectScriptOffset += len;
					}
					printf("START_TALK '%s'", str[0]);
				}
				break;
			case 30000: {
					int num = readNextScriptInt();
					printf("SWITCH_TO_SCENE(scene=%d)", num);
				}
				break;
			case 30010: {
					int num = readNextScriptInt();
					printf("SWITCH_TO_SCENE_COPY_BOXES(scene=%d)", num);
				}
				break;
			default:
				printf("Unhandled operator %d\n", op);
				exit(0);
			}
			printf("\n");
		}
		printf("\n");

		assert(objectScriptOffset == endOffset);
		++statementCount;
	}
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		FILE *fp = fopen(argv[1], "rb");
		if (fp) {
			objectScriptSize = getFileSize(fp);
			objectScriptData = (uint8 *)malloc(objectScriptSize);
			if (objectScriptData) {
				fread(objectScriptData, objectScriptSize, 1, fp);
				decodeObjectScript();
				free(objectScriptData);
				objectScriptData = 0;
			}
			fclose(fp);
		}
	}
	return 0;
}
