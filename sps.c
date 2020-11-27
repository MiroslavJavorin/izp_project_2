/**
 * Project 2 - simple spreadsheet editor 2
 * Subject IZP 2020/21
 * @Author Skuratovich Aliaksandr xskura01@fit.vutbr.cz
*/

// TODO в случае проблемы расширь таблицу

/* TODO make possible toquote a quotation mark if neccessarry
 *  change get_row(), delete get_cell()
 * */

//region includes
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
//endregion

// TODO delete globs
int error_line_global = 0;

//region macros
/*
 *  SHOWTAB  - shows operations
 *  SEPSBUG  - debug separators
 * */

/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */
#define CHECK_EXIT_CODE_IN_RUN_PROGRAM if(exit_code > 0)\
{\
    print_error_message(&exit_code);\
    return exit_code;\
}

/* checks exit code in a void function */
#define CHECK_EXIT_CODE if(*exit_code > 0){ return; error_line_global = __LINE__; }

/* checking for an alloc error TODO change to a function */
#define CHECK_ALLOC_ERR(arr) if(arr == NULL)\
{\
    *exit_code = W_ALLOCATING_ERROR;\
    error_line_global = __LINE__;\
    return;\
}

#define MEMBLOCK 10 /* allocation step */
#define MAXVAL -42
#define PTRNLEN 1001
#define NOF_TMP_SELS 10 /* max number of temo selections */

#define NEG(n) ( n = !n ); /* negation of bool value */

#define FREE(arr) if(arr) { free(arr); } /* check if a is not NULL */



//endregion

//region enums

enum erorrs
{
    W_ALLOCATING_ERROR = 1, /* error caused by a 'memory' function */
    W_SEPARATORS_ERROR,     /* wrong separators have been entered*/
    NUM_UNSUPARG_ERROR,     /* unsupported number of arguments */
    VAL_UNSUPCMD_ERROR,     /* wrong arguments in the commandline */
    LEN_UNSUPCMD_ERROR,
    NO_SUCH_FILE_ERROR,     /* no file with the entered name exists */
    Q_DONT_MATCH_ERROR      /* quotes dont match */
};

enum argpos
{
    /* position with no delim means first argument after name of the program */
    POSNDEL = 1,
    /* position with delim means first argument after delim string */
    POSWDEL = 3
};


typedef enum
{
    DAPROCOPT,
    TAEDITOPT,
    CHGSELOPT,
    TMPSELOPT,
    SEQENCOPT 
} argopt;

typedef enum
{
    /* set cursel to the 1-st column contains the given pattern */
    FIND,
    /* set curset to the column contains numberical value in the current cursel */
    MIN,
    MAX,

    /* clear all columns in cursel */
    CLEAR,

    IROW,
    AROW,
    DROW,
    ICOL,
    ACOL,
    DCOL
    //  TODO дополнить
} cmdopt;

//endregion

//region structures

/* contains information about delim string */
typedef struct carr_t
{
    int elems_c;   /* number of elements array contains */
    char *elems_v; /* dynamically allocated array of chars  */
    int length;    /* size of an array */
    bool isempty;
    bool isnum;
} carr_t;


typedef struct
{
    /* upper row, left column, lower row, right column */
    int row_1, col_1, row_2, col_2;
    cmdopt opt;
    char pttrn[PTRNLEN];   /* if there's a pattern in the command */
}cmd_t;

/* contains information about arguments user entered in commandline */
typedef struct
{
    carr_t seps;
    bool defaultsep; /* means ' ' is used as a searator */
    FILE *ptr;

    /* arguments themselfs */
    cmd_t currsel;       /* current selection */
    cmd_t curcmd;
    //cmd_t cmds[1000];       /* array with all commands(for functions goto etc) */
    int cmds_c;        /* number of commands entered by user*/
    cmd_t tmpsel[NOF_TMP_SELS]; /* array of selections */
} clargs_t;

/* contains information about a row */
typedef struct
{
    carr_t *cols_v; /* columns in the row */
    int cols_c;     /* number of filled cols */
    int length;     /* size of allocated memory*/
} row_t;

/* contains information about a table */
typedef struct
{
    row_t* rows_v; /* table contains rows which contain cells */
    int  row_c; /* number of rows of the table */
    int  length;  /* size of allocated memory*/
} tab_t;
//endregion


void print_tab(tab_t *t, carr_t *seps, FILE *ptr)
{
    (void) ptr;
    for(int row = 0; row < t->length; row++ )
    {
        for(int col = 0; col < t->rows_v[row].length; col++ )
        {
            if(col)
                fputc(seps->elems_v[0], stdout);

            for(int i = 0; i < t->rows_v[row].cols_v[col].elems_c; i++)
                fputc(t->rows_v[row].cols_v[col].elems_v[i], stdout);

        }
        putc('\n',stdout);
    }
}

//region FREE MEM
/* frees all memory allocated by a table_t structure s */
void free_table(tab_t *t)
{

#ifdef SHOWTAB
    printf("line %d FREE TABLE Table :len %d ?= rows %d\n", __LINE__,t->length, t->row_c);
#endif
    for(int row = t->length - 1; row >= 0; row--)
    {
#ifdef SHOWTAB
        printf("row %d with cols,| len = %d ?= colc %d\n", row, t->rows_v[row].length, t->rows_v[row].cols_c );
#endif
        for(int col = t->rows_v[row].length - 1; col >= 0; col--)
        {
#ifdef SHOWTAB
            printf(" %d", col);
#endif
            FREE(t->rows_v[row].cols_v[col].elems_v)
        }
        FREE(t->rows_v[row].cols_v)
#ifdef SHOWTAB
        printf(" freed\n");
#endif
    }
    FREE(t->rows_v)
#ifdef SHOWTAB
    printf("table freed\n");
#endif
}

/* frees all memory allocated by a clargs_t structure s */
void free_clargs(clargs_t *s)
{
    if(s->seps.elems_v)
        FREE(s->seps.elems_v)
    if(s->ptr)
        fclose(s->ptr);
}

void clear_data(tab_t *t, clargs_t *clargs)
{
    free_table(t);
    free_clargs(clargs);
}
//endregion

//region CONSTRUCTORS

void carr_ctor(carr_t *arr, int *exit_code)
{
    arr->elems_v = (char*)calloc(MEMBLOCK, sizeof(char)); // FIXME if someting foesnt work change to malloc
    CHECK_ALLOC_ERR(arr->elems_v)

    arr->length  = MEMBLOCK;
    arr->elems_c = 0;
    arr->isempty = true;
    arr->isnum   = false;
}

/* allocating a row of size MEMBLOCK */
void row_ctor(row_t *r, int *exit_code)
{
    r->cols_v = (carr_t*)malloc( MEMBLOCK * sizeof(carr_t));
    CHECK_ALLOC_ERR(r->cols_v)

    /* allocate cols */
    for(int cols = 0; cols < MEMBLOCK; cols++)
    {
        carr_ctor(&r->cols_v[cols], exit_code);
        CHECK_EXIT_CODE
    }
    r->length = MEMBLOCK;
    r->cols_c = 0;
}

/* creates a table by creating rows and columns */
void table_ctor(tab_t *t, int* exit_code)
{
    /* allocate thw new table */
    t->rows_v = (row_t*)malloc(MEMBLOCK * sizeof(row_t));
    CHECK_ALLOC_ERR(t->rows_v)

    /* allocate rows */
    for(int row = 0; row < MEMBLOCK; row++)
    {
        row_ctor(&t->rows_v[row], exit_code);
        CHECK_EXIT_CODE
    }
    t->row_c = 0;
    t->length = MEMBLOCK;
}
//endregion

//region ARRAYS WITH CHARS FUNCS


void iscellnum(carr_t *cell)
{
    char* junk;
    cell->isnum = true;

    strtod(cell->elems_v, &junk);

    if(junk[0])
        cell->isnum = false;
}

/**
 * Copies string src to string string dst
 *
 * @param array of chars to copy to
 * @param array of chars to copy from
 * @return false If len of dst < len of src
 *         true if copied succesfully
 */
bool strcop(char* dst, char* src)
{
    int dstlen = (int)strlen(dst);
    if(dstlen < (int)strlen(src))
        return false;

    int i = 0;
    while(i < dstlen)
    {
        dst[i] = src[i];
        i++;
    }
    return true;
}

/**
 * String beginswith string
 * @param str
 * @param ptrn
 * @return false if pattern is longer than str
 *            or first strlen(ptrn) chars aren't equal to str
 *          true if string begins with ptrn
 */
bool strbstr(char *str, char *ptrn)
{
    int ptrnl = (int)strlen(ptrn);
    if(ptrnl > (int)strlen(str))
    {
        return false;
    }else
    {
        int i = 0;
        while(i < ptrnl)
        {
            if(str[i] != ptrn[i])
                return false;
            i++;
        }
        return true;
    }
}

/**
 *  Adds a character to an array.
 *  If there is no more memory in the array allocate new memory
 *
 * @param arr        dynamically allocated array.
 * @param item       an item to add to an array
 * @param exit_code  can be changed if array reallocated unsuccesfully
 */
void a_carr(carr_t *arr,const char item, int *exit_code)
{
    /* if there is no more space for the new element add new space */
    if(arr->elems_c + 1 >= arr->length )
    {
        arr->length += MEMBLOCK;
        arr->elems_v = (char*)realloc(arr->elems_v, arr->length  * sizeof(char));
        CHECK_ALLOC_ERR(arr->elems_v)
    }
    arr->isempty = false;
    arr->elems_v[arr->elems_c++] = item;
}

///** TODO deleteme
// * Check if set contains given item
// *
// * @return true if given set contains the item
// *         false otherwise
// */
//bool set_contains(carr_t *set, const char *item)
//{
//    for(int i = 0; i < set->elems_c; i++)
//        if(set->elems_v[i] == *item)
//             return true;
//
//     return false;
//}

/**
 *  Adds an item to an array if this item is not represented in the array
 *
 * @param set An array where to add an item
 * @param size Size of an array where to add an item. If item is adds size will be increased
 * @param item A character to add to an array
 * @param exit_code Exit code is changed only if the error while allocating memory has been occurred
 *
 * @return
 *         true, if item has been added to an array
 *         false, if item is already in the array or if error while allocating memory has been occurred
 */
void set_add_item(carr_t *set, char item, int *exit_code)
{
    if(strchr(set->elems_v, item) == NULL)
    {
        a_carr(set, item, exit_code);
        CHECK_EXIT_CODE
    }
}

///** TODO deleteme
// * Returns the number of occurancef of the given char in the given array
// *
// * @param arr
// * @param charchar
// * @return
// *//*
//int stroc(char *str, char charchar)
//{
//    int n = 0;
//    int i = 0;
//    int slen = (int)strlen(str);
//    while(i < slen)
//    {
//        if(charchar == str[i])
//            n++;
//        i++;
//    }
//    return n;
//}*/


//endregion

//region TRIMS
/* trims an array of characters */
void cell_trim(carr_t *arr, int *exit_code)
{
    if(!arr->elems_v)
    {
        printf("\t\t\t э кумыс\n");
        arr->elems_v = (char*)calloc(MEMBLOCK ,sizeof(char));
        exit(228);
    }
#ifdef SHOWTAB
    printf("\t\t\tlen %d elems %d\n", arr->length, arr->elems_c);
#endif
    if(arr->length >= arr->elems_c + 1)
    {
        /* reallocate for elements and terminating 0 */
        if(!arr->elems_c)
            arr->elems_v = (char *)realloc(arr->elems_v, (arr->elems_c + 1) * sizeof(char));
        else
            arr->elems_v = (char *)realloc(arr->elems_v, arr->elems_c * sizeof(char));

        CHECK_ALLOC_ERR(arr->elems_v)

        arr->length = (arr->elems_c) ? arr->elems_c : 1;
    }
}

void row_trim(row_t *row, int *exit_code)
{
    /* trim all cells in the row */
#ifdef SHOWTAB
    printf("(%d) cols -> %d  len -> %d . \n", __LINE__, row->cols_c, row->length );
#endif
    for(int cell = row->cols_c; cell >= 0; cell-- )
    {
        cell_trim(&row->cols_v[row->cols_c], exit_code);
        CHECK_EXIT_CODE
    }

    for(int i = row->length - 1; i > row->cols_c; i--)// FIXME ISSUE here
    {    FREE(row->cols_v[i].elems_v)}

    row->cols_v = (carr_t*)realloc(row->cols_v, (row->cols_c + 1) * sizeof(carr_t));
    CHECK_ALLOC_ERR(row->cols_v)
    row->length = row->cols_c + 1;
}

/* trims a table by reallocating rows and cols */
void table_trim(tab_t *t, int *exit_code)
{
    int row;
    /* free unused rows that have been alloacated */
    if(t->row_c != t->length)
    {
        for(row = t->rows_v->length - 1; row >= t->row_c; row--)
        {
            for(int col = t->rows_v[row].length - 1; col >= 0; col--)
                FREE(t->rows_v[row].cols_v[col].elems_v)
            FREE(t->rows_v[row].cols_v)
        }

        t->rows_v = (row_t *)realloc(t->rows_v, t->row_c * sizeof(row_t));
        CHECK_ALLOC_ERR(t->rows_v)
        t->length = t->row_c;
    }
    /* trim each row */
    for(row = 0; row < t->row_c; row++)
    {
#ifdef SHOWTAB
        printf("(%d)  row %d len %d\n", __LINE__,row, t->rows_v[row].length);
#endif
        row_trim(&t->rows_v[row], exit_code);
        CHECK_EXIT_CODE
    }

}
//endregion

//region GETTERS

void set_cursel(clargs_t *clargs, int row1, int col1, int row2, int col2, int *exit_code)
{
    // TODO если в таблице меньше рядов/колонок, расширь таблицу
    if(row1 > row2 || col1 > col2)
    {
        *exit_code = VAL_UNSUPCMD_ERROR;
        return;
    } else
    {
        clargs->currsel.row_1 = row1;
        clargs->currsel.col_1 = col1;
        clargs->currsel.row_2 = row2;
        clargs->currsel.col_2 = col2;
    }
    printf("(%d) cursel changed: from %d %d to  %d %d\n", __LINE__, row1 , col1, row2, col2 );
}

/* sets new cursel to the column with min/max numerical value from the cursel */
void getmcol(tab_t *t, clargs_t *clargs, int *exit_code)
{
    int row_fr = (clargs->currsel.row_1 == MAXVAL) ? t->row_c - 1 : clargs->currsel.row_1 - 1;
    int row_to = (clargs->currsel.row_2 == MAXVAL) ? t->row_c - 1 : clargs->currsel.row_2 - 1;
    int col_fr = 0;
    int col_to = 0;
    int m_row = 0, m_col = 0; /* initialize positions */

    double m_val = 0; /* if there will be no numbers change nothing */
    double tmp_val;
    bool set = false;

    for( ;row_fr <= row_to; row_fr++)
    {
        col_fr = (clargs->currsel.col_1 == MAXVAL) ? t->rows_v[row_fr].cols_c - 1 : clargs->currsel.col_1 - 1;
        col_to = (clargs->currsel.col_2 == MAXVAL) ? t->rows_v[row_fr].cols_c - 1 : clargs->currsel.col_2 - 1;
#ifdef SELECT
        printf("(%d) col_fr = %d col_to = %d\n", __LINE__, col_fr, col_to);
#endif
        for(; col_fr <= col_to; col_fr++ )
        {
            if(t->rows_v[row_fr].cols_v[col_fr].isnum)
            {
                tmp_val = strtod(t->rows_v[row_fr].cols_v[col_fr].elems_v, NULL);
                if(!set)
                {
                    m_val = tmp_val;
                    m_row = row_fr, m_col = col_fr;
                    set = true;
#ifdef SELECT
                    printf("(%d)    m_val %lf m_row = %d m_col = %d\n", __LINE__, m_val, m_row, m_col);
#endif
                }
                else if(clargs->curcmd.opt == MIN && tmp_val < m_val)
                {
#ifdef SELECT
                    printf("(%d)        min m_val %lf m_row = %d m_col = %d\n", __LINE__, m_val, m_row, m_col);
#endif
                    m_val = tmp_val;
                    m_row = row_fr, m_col = col_fr;
                }
                else if(clargs->curcmd.opt == MAX && tmp_val > m_val)
                {
#ifdef SELECT
                    printf("(%d)        max m_val %lf m_row = %d m_col = %d\n", __LINE__, m_val, m_row, m_col);
#endif
                    m_val = tmp_val;
                    m_row = row_fr, m_col = col_fr;
                }
            }
        }
    }
    if(set)
    {
        /* actually there is no need to change exit_code */
        set_cursel(clargs, m_col, m_col, m_row, m_col, exit_code);
    }
}

/* returns true is newline or eof reached */
void get_nums(carr_t *command, clargs_t *clargs, int *exit_code)
{
    char *token;
    char *ptr;
    int nums[4] = {0};
    int i = 0;
    /* get the first token */
    token = strtok(command->elems_v, ",");

    /* walk through other tokens */
    for(; i < 4 && token != NULL; i++)
    {
        /* add a number to an array */
        nums[i] = (int)(strtol(token, &ptr, 10));

        if(nums[i] <= 0)
        {
            if(strcmp(ptr, "-") != 0)
            {
                *exit_code = VAL_UNSUPCMD_ERROR;
                error_line_global = __LINE__;
                return;
            }
            nums[i] = MAXVAL;
        }
        token = strtok(NULL, ",");
    }

    set_cursel(clargs, nums[0], nums[1], nums[(i == 3) ? 2 : 0], nums[(i == 3) ? 3 : 0], exit_code);
    CHECK_EXIT_CODE
}

bool get_cell(carr_t *col, clargs_t *clargs, int *exit_code)
{
    char buff_c;
    bool quoted = false;
    for(col->elems_c = 0; ;)
    {
        buff_c = (char)fgetc(clargs->ptr);
        if(buff_c == '\n' || feof(clargs->ptr))
        {
            if(quoted) *exit_code = Q_DONT_MATCH_ERROR;
#ifdef SHOWTAB
            printf("(%d)  \t\t\t%s\n", __LINE__, col->elems_v);
#endif
            return true;
        }

        /* start or end a quotation, where 34 is a quotation mark */
        else if(buff_c == 34)
        {
            NEG(quoted)
        }

        /* if the char is a separator and it is not quoted */
        else if(strchr(clargs->seps.elems_v, buff_c) != NULL && !quoted) // FIXME если не рабптает, использовать
            // set_contains
        {
#ifdef SHOWTAB
            printf("(%d)  \t\t\t%s\n", __LINE__, col->elems_v);
#endif
            return false;
        }
        a_carr(col, buff_c, exit_code);
    }
}

/* gets a row from the file and adds it to the table */
void get_row(row_t *row, clargs_t *clargs, int *exit_code)
{
    bool end;
    for(row->cols_c = 0; !feof(clargs->ptr); row->cols_c++ )
    {
        if(row->cols_c == row->length)
        {
            row->length += MEMBLOCK;
            row->cols_v = (carr_t *)realloc(row->cols_v, row->length * sizeof(carr_t));
            CHECK_ALLOC_ERR(row->cols_v)

            /* create a col */
            carr_ctor(&row->cols_v[row->cols_c], exit_code);
            CHECK_EXIT_CODE
        }
#ifdef SHOWTAB
        printf("(%d)  \t\tcol %d\n", __LINE__, row->cols_c);
#endif
        end = get_cell(&row->cols_v[row->cols_c], clargs, exit_code);
        CHECK_EXIT_CODE

        /* check if the cell is a number and set true or false to the cell */
        iscellnum(&row->cols_v[row->cols_c]);

        if(end)
        {
#ifdef SHOWTAB
            printf("(%d)  \t\tlen %d\n", __LINE__, row->length);
#endif
            return;
        }
    }
}

void get_table(const int *argc, const char **argv, tab_t *t, clargs_t *clargs, int *exit_code)
{
    //TODO check if file is not empty
    if((clargs->ptr = fopen(argv[*argc - 1], "r+")) == NULL)
    {
        *exit_code = NO_SUCH_FILE_ERROR;
        return;
    }

    /* if file has been opened successfully */
    else
    {
        /* allocate a table */
        table_ctor(t, exit_code);
        CHECK_EXIT_CODE

        /* write the table to the structure */
        for(t->row_c = 0; !feof(clargs->ptr); t->row_c++ )
        {
            /* realloc the table to the new size */
            if(t->row_c == t->length)
            {
                t->length += MEMBLOCK;
                t->rows_v  = (row_t*)realloc(t->rows_v, t->length * sizeof(row_t));
                CHECK_ALLOC_ERR(t->rows_v)
                /* create a row */
                row_ctor(&t->rows_v[t->row_c], exit_code);
            }
#ifdef SHOWTAB
            printf("(%d)  row %d\n", __LINE__, t->row_c);
#endif
            get_row(&t->rows_v[t->row_c], clargs, exit_code);
            CHECK_EXIT_CODE
        }

        table_trim(t, exit_code);
        CHECK_EXIT_CODE
    }
}
//endregion

//region COMMANDLINE ARGS PARSING

void daporc_tmp_vars(carr_t *command, clargs_t *clargs, tab_t *t, int *exit_code)
{
    (void)command;
    (void)clargs;
    (void)t;
    (void)exit_code;
    /* if command is FIND and string to  */
    if(strbstr(command->elems_v, "[find "))
    {
        clargs->currsel.opt = FIND;
        if(!strcop(clargs->currsel.pttrn, command->elems_v))
        {
            *exit_code = LEN_UNSUPCMD_ERROR;
            error_line_global = __LINE__;
        }
        return;
    }
}

/**
 *  Initialize a command from argument
 *  If the given command is a selection,
 *   then change the current selection to the new given selection
 * @param command an array with a command
 * @param clargs structure with commandline aruments, including an array
 *        with commands and current selection
 * @param t structure with a table to call table editing functions
 * @param exit_code
 */
void cng_sel_tab_ed(carr_t *command, clargs_t *clargs, tab_t *t, int *exit_code)
{
    /* number of occurances */
    char *currcom = calloc(command->elems_c, sizeof(char));
    CHECK_ALLOC_ERR(currcom)

    /* add parameters to the structure */
   //int nums_c = sscanf(command->elems_v, "[%d,%d]", &clargs->currsel.row_1, &clargs->currsel.col_1);
    if     (!strcmp(command->elems_v, "irow")) {clargs->curcmd.opt = IROW;}
    else if(!strcmp(command->elems_v, "arow")) {clargs->curcmd.opt = AROW;}
    else if(!strcmp(command->elems_v, "drow")) {clargs->curcmd.opt = DROW;}
    else if(!strcmp(command->elems_v, "icol")) {clargs->curcmd.opt = ICOL;}
    else if(!strcmp(command->elems_v, "acol")) {clargs->curcmd.opt = ACOL;}
    else if(!strcmp(command->elems_v, "dcol")) {clargs->curcmd.opt = DCOL;}
    else if(!strcmp(command->elems_v, "clear")){clargs->curcmd.opt = CLEAR;}

    else if(!strcmp(command->elems_v, "min"))
    {
        clargs->curcmd.opt = MIN;
        getmcol(t, clargs); /* set column in range of the selection by numerical value */
        printf("(%d) min col has been set r %d c %d\n", __LINE__, clargs->currsel.row_1, clargs->currsel.col_1);
        return;
    }
    else if(!strcmp(command->elems_v, "max"))
    {
        clargs->curcmd.opt = MAX;
        getmcol(t, clargs); /* set column in range of the selection by numerical value */
        printf("(%d) max col has been set r %d c %d\n", __LINE__, clargs->currsel.row_1, clargs->currsel.col_1);
        return;
    }
    else if(!strcmp(command->elems_v, "_"))
    {
        // TODO refresh selection from temp var
        printf("%d [_]", __LINE__);
    }

    /* if there's a selection command */
    else
    {
        get_nums(command, clargs, exit_code);
        CHECK_EXIT_CODE
    }

    FREE(currcom)
}

/**
 *  For each agrument (selection or processing command)
 *  copies everything before ; or before \0 t to the temp array,
 *  then parses the temp array and calls the function optionally
 *
 * @param dst ar array where to copy a command
 * @param src a commandline argument
 * @param exit_code can be changed if the error occured
 */
void parse_arg(carr_t *dst, const char *arg, clargs_t *clargs, tab_t *t, int *exit_code)
{
    int arglen = (int)strlen(arg);
    bool quoted = false;
    /* set a selection to the first cell */
    set_cursel(clargs, 1, 1, 1, 1, exit_code);

    /* write argument to an array */
    for(int p = 0; p < arglen; p++)
    {
        if(arg[p] == '\"')
        {
            NEG(quoted)
        }

        else if(arg[p] == '[' || arg[p] == ']')
        {
            continue;
        }
        
        /* if the end of the argument reached */
        else if(arg[p] == ';' && !quoted)
        {
            cell_trim(dst, exit_code);
            CHECK_EXIT_CODE
            printf("(%d) %s\n",__LINE__, dst->elems_v);

            /* edit structure of the table or change current selection */
            if(strchr(dst->elems_v, ' ') == NULL)
                cng_sel_tab_ed(dst, clargs, t, exit_code);
            
            /* init and call data processing functions or process temp variables */
            else
                daporc_tmp_vars(dst, clargs, t, exit_code);

            CHECK_EXIT_CODE

            dst->elems_c = 0;
            dst->isempty = true;
            memset(dst->elems_v, 0, dst->elems_c); // fill an array with 0s
        }

        a_carr(dst, arg[p], exit_code);
        CHECK_EXIT_CODE
    }
}

/**
 * Initializes an array of entered separators(DELIM)
 *
 * @param argc Number of commandline arguments
 * @param argv An array with commandline arguments
 * @param exit_code exit code to change if an error occurred
 */
void init_separators(const int *argc, const char **argv, clargs_t *clargs, int *exit_code)
{
    //region variables
    clargs->seps.length = 0;
    carr_ctor(&clargs->seps, exit_code);
    CHECK_EXIT_CODE
    int k = 0;
    //endregion

    /* if there is only name, functions and filename */
    if(*argc >= 3 && strcmp(argv[1], "-d") != 0)
    {
        a_carr(&clargs->seps, ' ', exit_code);
        clargs->defaultsep = true;
        CHECK_EXIT_CODE
    }

        /* if user entered -d flag and there are function names and filename in the commandline */
    else if(*argc >= 5 && !strcmp(argv[1], "-d"))
    {
        clargs->defaultsep = false;
        while(argv[2][k])
        {
            /* checks if the user has not entered characters that will lead to undefined program behavior */
            if(argv[2][k] == 10 || argv[2][k] == 92 || argv[2][k] == 34)
            {
                *exit_code = W_SEPARATORS_ERROR;
                return;
            }
            set_add_item(&(clargs->seps), argv[2][k], exit_code);
            CHECK_EXIT_CODE
            k++;
        }
    }

        /* If number of arguments is less than or equal to 2 which means t
         * here are only name of the program and filename (optional) */
    else if(*argc <= 4)
    {
        *exit_code = NUM_UNSUPARG_ERROR;
        return;
    }
#ifdef SEPSBUG
    printf("line %d separators -> ", __LINE__);
    for(int i = 0; i < clargs->seps.elems_c; i++ )
        printf("%c",clargs->seps.elems_v[i]);
    putchar(10);
#endif
}
//endregion

void parse_clargs_proc_tab(const int *argc, const char **argv, tab_t *t, clargs_t *clargs, int *exit_code)
{
    /* by default has value 1, which means there was no delim in the command line */
    int clarg = (clargs->defaultsep) ? POSNDEL : POSWDEL;
    carr_t barg; /* buffer commandline arg */
    carr_ctor(&barg, exit_code);
    CHECK_EXIT_CODE

    /* means there is no arguments entered in the command line */
    if(clarg == *argc - 1)
    {
        *exit_code = NUM_UNSUPARG_ERROR;
        return;
    }

    /* copies an argument to the array */
    parse_arg(&barg, argv[clarg], clargs, t, exit_code);
    CHECK_EXIT_CODE

    FREE(barg.elems_v)
}

void process_table(clargs_t *clargs, tab_t *t, const int *exit_code)
{
    (void) clargs;
    (void) exit_code;
    (void) t;
}

void print_error_message(const int *exit_code)
{
    /* an array with all error messages */
    char *error_msg[] =
    {
            "Twenty afternoons in utopia",
            "Entered separators are not supported buy the program",
            "You've entered too few arguments",
            "There is no file with this name",
            "Why?! And we light up the sky!!!",
            "Watch me die",
            "Wake me up yesterdays morning",
            "Choke me by a spagetti"
    };

    /* print an error message to stderr */
    fprintf(stderr, "Error %d: %s occured on the line %d.\n", *exit_code, error_msg[*exit_code - 1], error_line_global);
}

/*
 * Runs the program: initializes structures, then initializes commandline arguments, then processes the table
 * After every initializing checks for an exit_code to return it if the error has been occurred
 * Frees memory that was allocated while program was running.
 */
int run_program(const int argc, const char **argv)
{
    //region arguments
    int exit_code = 0;
    tab_t  t;
    clargs_t clargs;
    //endregion

    /* initialize separators from commandline */
    init_separators(&argc, argv, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* adds table to the structure */
    get_table(&argc, argv, &t, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* parse commandline arguments and process table for them */
    parse_clargs_proc_tab(&argc, argv, &t, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    print_tab(&t, &clargs.seps, clargs.ptr);

    clear_data(&t, &clargs);

    /* exit_code is -1 means table has been processed successfully */
    return (exit_code == -1) ? 0 : exit_code;
}

/* Prints documentation, Author */
void print_documentation()
{
    printf("Kiss an ugly turtle to make it cry");
}

int main(const int argc, const char **argv)
{
    /* print documentation */
    if(argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))
    {
        print_documentation();
        return 0;
    }

    /* run the program */
    return run_program(argc, argv);
}
