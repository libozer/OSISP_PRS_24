#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <ctype.h>
#include <ncurses.h>
#define max_size_input 10000
#define max_size_word 46

int replace = 0;

int checkWord(wchar_t *word, int lang)
{
    wchar_t *buffer, *temp;
    FILE *dict;

    buffer = (wchar_t *) calloc(max_size_word, sizeof(wchar_t));
    if(!buffer)
    {
        fprintf(stderr, "Can't open file\n");
        return -1;
    }

    temp = (wchar_t *) calloc(max_size_word, sizeof(wchar_t));
    if(!temp)
    {
        fprintf(stderr, "Can't open file\n");
        return -1;
    }

    wcscpy(temp, word);

    if(lang)
        dict = fopen("./english.txt", "r");
    else
        dict = fopen("./russian.txt", "r");

    if(dict == NULL)
    {
        fprintf(stderr, "Can't open file\n");
        return -1;
    }

    while(fgetws(buffer, max_size_word, dict))
    {
        if(feof(dict))
            break;

        buffer[wcscspn(buffer, L"\n\r%")] = L'\0';

        temp[0] = towlower(temp[0]);

        if(!wcscmp(temp, buffer))
            return 1;
    }

    free(buffer);
    free(temp);
    fclose(dict);

    return 0;
}

void switchLayout(char *layout)
{
    Display *display;
    char *display_name = "", *group;
    int device = XkbUseCoreKbd, i = 0, counter = 0;    //используется основная клавиатура
    Atom current;

    display = XOpenDisplay(display_name);
    if(display == NULL)
    {
        fprintf(stderr, "Cannot connect to display\n");
        return;
    }

    XkbDescRec *keyboard_description = XkbAllocKeyboard();    //инициализация структруры, хранящей информацию о клавиатуре
    if(keyboard_description == NULL)
    {
        fprintf(stderr, "Can't get keyboard description\n");
        return;
    }
    keyboard_description->dpy = display;    //установка указателя на дисплей, с которым связана клавиатура

    if(device != XkbUseCoreKbd)
        keyboard_description->device_spec = device;    //если было указано конкретное устройство

    XkbGetControls(display, XkbAllControlsMask, keyboard_description);    //получение всей доступной информации о состоянии и управлении клавиатурой и сохранение ее в поле ctrs
    XkbGetNames(display, XkbSymbolsNameMask, keyboard_description);    //получение имена символов клавиш и сохранение их в поле names
    XkbGetNames(display, XkbGroupNamesMask, keyboard_description);    //получение имен групп раскладок клавиатуры и сохранение их в поле names

    Atom *layout_groups = keyboard_description->names->groups;    //массив групп раскладок клавиатуры
    if(keyboard_description->ctrls != NULL)
        counter = keyboard_description->ctrls->num_groups;
    else    //если отсутствует информация о количестве раскладок
    {
        while(counter < XkbNumKbdGroups && layout_groups[counter] != 0)
            counter++;
    }

    for(i=0; i<counter; i++)
    {
        if((current = layout_groups[i]) != None)
        {
            group = XGetAtomName(display, current);

            if(group == NULL)
                continue;
            else
            {
                if(strncmp(layout, group, strlen(layout)) == 0)
                {
                    XkbLockGroup(display, device, i);    //установить нужную раскладку

                    XkbFreeKeyboard(keyboard_description, XkbAllComponentsMask, True);
                    XFree(group);

                    XCloseDisplay(display);

                    return;
                }
            }
        }
    }
}

int main()
{
    wchar_t *input, *changed_input, *word, *temp, *ptr, separators[] = L" ,.?!", sym[2] = {L'\0'}, choice = L'y';
    long int start = 0;
    size_t j =0;
    void checkSpelling(wchar_t *);

    setlocale(LC_ALL, "en_US.UTF-8");

    initscr();
    WINDOW *win = newwin(10, 30, 0, 0);

    input = (wchar_t *) calloc(max_size_input, sizeof(wchar_t));
    if(!input)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    changed_input = (wchar_t *) calloc(max_size_input, sizeof(wchar_t));
    if(!changed_input)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    ptr = (wchar_t *) calloc(max_size_input, sizeof(wchar_t));
    if(!ptr)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    word = (wchar_t *) calloc(max_size_input, sizeof(wchar_t));
    if(!word)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    temp = (wchar_t *) calloc(max_size_input, sizeof(wchar_t));
    if(!temp)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    printw("Welcome to Language Conversion Program\n");
    printw("Please input a text to be processed.\n\n");
    printw("Press any key to begin...");
    switchLayout("English (US)");
    wrefresh(win);
    getch();
    endwin();

    while(choice == L'y' || choice == L'Y' || choice == L'н' || choice == L'Н')
    {
        system("clear");

        fgetws(input, max_size_input, stdin);
        input[wcscspn(input, L"\n")] = '\0';

        wcscpy(changed_input, input);

        word = wcstok(input, separators, &ptr);
    //    word[wcscspn(word, L"\n")] = '\0';
        while(word != NULL)    //пока не достигнут конец строкиы
        {
            start = word - input;    //запоминание позиции начала слова
            j = start + wcslen(word);    //позиция после токена

            if(wcschr(L",.", changed_input[j]) != NULL)    //если следующий за токеном символ может оказаться русскими ю или б
            {
                if(changed_input[j+1] != L' ' && changed_input[j+1] != L'\0')    //если это не знак препинания (в середине слова)
                {
                    while(changed_input[j+1] != L' ' && changed_input[j+1] != L'\0')
                    {
                        sym[0] = changed_input[j];
                        wcscat(word, sym);    //добавление оставшейся части слова после б/ю
                        j++;
                    }

                    wcscpy(input, changed_input);
                    wcstok(NULL, separators, &ptr);    //обеспечение перехода к следующему токену
                }

                else    //если это может оказаться словом, оканчивающимся на б/ю
                {
                    wcscpy(temp, word);

                    sym[0] = changed_input[j];
                    wcscat(temp, sym);    //включение символа ./, в состав слова

                    checkSpelling(temp);
                    if(checkWord(temp, 0))    //если это слово, оканчивающееся на б/ю
                        wcscat(word, sym);
                }
            }

            checkSpelling(word);

            if(replace)
                for(size_t i=start, j=0; j<wcslen(word); i++, j++)
                    changed_input[i] = word[j];    //замена обработанного слова в выводе

            word = wcstok(NULL, separators, &ptr);    //переход к следующему токену
        }

        setlocale(LC_ALL, "en_US.UTF-8");

        system("clear");
        wprintf(L"Processed Text:\n%ls\n\n", changed_input);
        fflush(stdout);

        wprintf(L"Do you want to continue? (Y/N): ");
        choice = getwchar();
        getwchar();
    }

    return 0;
}

void checkSpelling(wchar_t *word)
{
    int english = 0, russian = 0;
    void replaceFragment(wchar_t *, int);

    for(size_t i=0; i<wcslen(word); i++)
    {
        setlocale(LC_ALL, "");

        if((isalpha(word[i]) && (word[i] >= 'a' && word[i] <= 'z')) || (word[i] >= 'A' && word[i] <= 'Z'))
            english++;

        else
            russian++;
    }

    if(english && !russian)
    {
        if(checkWord(word, 1))
        {
            replace = 0;
            return;
        }
        else
        {
            replace = 1;
            replaceFragment(word, 1);
        }
    }

    else if(!english && russian)
    {
        if(checkWord(word, 0))
        {
            replace = 0;
            return;
        }
        else
        {
            replace = 1;
            replaceFragment(word, 0);
        }
    }

    else if(russian && english)
    {
        replace = 1;

        replaceFragment(word, 0);
        if(!checkWord(word, 1))
        {
            replaceFragment(word, 1);
            checkWord(word, 0);
        }
    }
}

void replaceFragment(wchar_t *word, int lang)
{
    wchar_t en_layout[] = L"§qwertyuiop[]asdfghjkl;'zxcvbnm,.§QWERTYUIOP[]ASDFGHJKL;'ZXCVBNM,.",
    ru_layout[] =         L"ёйцукенгшщзхъфывапролджэячсмитьбюЁЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮ";

    setlocale(LC_ALL, "en_US.UTF-8");

    for(size_t i=0; i<wcslen(word); i++)
    {
        if(lang == 1)
        {
            for(size_t j=0; j<wcslen(en_layout); j++)
                if(word[i] == en_layout[j])
                {
                    word[i] = ru_layout[j];
                    break;
                }

            switchLayout("Russian");
        }

        else if(lang == 0)
        {
            for(size_t j=0; j<wcslen(ru_layout); j++)
                if(word[i] == ru_layout[j])
                {
                    word[i] = en_layout[j];
                    break;
                }

            switchLayout("English (US)");
        }
    }
}

