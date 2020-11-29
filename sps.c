/**
 * Project 2 - simple spreadsheet editor 2
 * Subject IZP 2020/21
 * @Author Skuratovich Aliaksandr xskura01@fit.vutbr.cz
*/

// TODO in case of an error free the table and cl

/* TODO make possible to quote a quotation mark if neccessarry
 *  change get_row(), delete get_cell()
 * */

//region includes
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
//endregion

// FIXME delete globs
int error_line_global = 0;

//region macros
/*
 *  SHOWTAB  - shows operations
 *  SEPSBUG  - debug separators
 * */

/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */
#define CURRSEL cl->currsel

#define CHECK_EXIT_CODE_IN_RUN_PROGRAM if(exit_code > 0)\
{\
    print_error_message(&exit_code);\
    return exit_code;\
}

/* checks exit code in a void function */
#define CHECK_EXIT_CODE if(*exit_code > 0){ printf("(%d) err\n",__LINE__);error_line_global = __LINE__;return; }

/* checking for an alloc error FIXME опаасно, поменяй на функцию */
#define CHECK_ALLOC_ERR(arr) if(arr == NULL)\
{\
    *exit_code = W_ALLOCATING_ERR;\
    CHECK_EXIT_CODE\
}


#define MEMBLOCK     10 /* allocation step */
#define MAXVAL      -42
#define ALLVALS     -13
#define PTRNLEN      1001
#define NOF_TMP_SELS 10 /* max number of temo selections */

#define NEG(n) ( n = !n ); /* negation of bool value */
#define FREE(arr) if(arr != NULL) { free(arr); } /* check if a is not NULL */
//endregion

//region enums
typedef enum
{
    NCOLS,
    WCOLS
} rtrim_opt; /* row trim option for trimming function */

enum erorrs
{
    W_ALLOCATING_ERR = 1, /* error caused by a 'memory' function */
    
    W_SEPARATORS_ERR,     /* wrong separators have been entered  */
    VAL_UNSUPARG_ERR,     /* unsupported number of arguments     */
//    VAL_UNSUPCMD_ERR,     /* wrong arguments in the cmdline  */
    NUM_UNRECARG_ERR,     /* unrecognized argument               */
    NO_SUCH_FILE_ERR,     /* no file with the entered name exists */
    LEN_UNSUPCMD_ERR,     /* unsupported length of the cmd   */
    Q_DONT_MATCH_ERR,      /* quotes dont match                   */
    UNEXPD_BEHAV_ERR
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
    NOPROC,
    SET,FIND,
    MIN, MAX, CLEAR,
    IROW, AROW, DROW, ICOL, ACOL, DCOL
    //  TODO дополнить
} proc_opt_t;

typedef enum
{
    NOSEL,
    WHOLE, /* select the whole table                               */
    CELL,  /* select only one cell                                 */
    CELLS, /* select a window(row, col, or a set of rows and cols) */
    TMP,
} sel_opt_t;

typedef enum
{
    NOPT,
    SEL,
    PRC,
} cmd_opt_t;
//endregion

//region structures

/* contains information about delim string */
typedef struct
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
    cmd_opt_t   cmd_opt; /* defines processing option or selection option */

    sel_opt_t   sel_opt;
    proc_opt_t  proc_opt;
    char        pttrn[PTRNLEN];   /* if there's a pattern in the cmd */
    bool        issel;
    bool        isset;
}cmd_t;

/* contains information about arguments user entered in cmdline */
typedef struct
{
    carr_t seps;
    bool defaultsep; /* means ' ' is used as a separator */
    FILE *ptr;

    /* arguments themselfs */
    cmd_t cmds[1001]; /* array with all cmds(for functions goto etc) */
    int currsel;      /* index of the current selectinon */
    int cellsel;      /* index of the current cell seleciton */
    int cmds_c;       /* number of entered cmds. Represents a position of a current cmd */

    int tmpsel[NOF_TMP_SELS]; /* array with ppositions of temp selections */
} cl_t;

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
    int  row_c;    /* number of rows of the table */
    int  length;   /* size of allocated memory*/
    int  col_c;    /* represents number of columns after trimming */
} tab_t;
//endregion


void print_tab(tab_t *t, carr_t *seps, FILE *ptr)
{
    (void) ptr;
#ifdef SHOWTAB
    printf("\nPRINT_TAB\n%d-----------------------\n", __LINE__);
#endif
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
#ifdef SHOWTAB
    printf("%d-----------------------\n", __LINE__);
#endif
}

/* n1 is less than n2*/
bool lessthan(int n1, int n2)
{
    if(n1 == MAXVAL || n1 == ALLVALS)
    {
        return false;
    }
    else if(n2 == MAXVAL || n2 == ALLVALS)
    {
        return true;
    }
    else return (bool)(n1 < n2);
}

//region FREE MEM
// TODO deletme
///* frees all memory allocated by a table_t structure s */
//void free_table1(tab_t *t)
//{
//#ifdef SHOWTAB
//    printf("line %d FREE TABLE Table :len %d ?= rows %d\n", __LINE__,t->length, t->row_c);
//#endif
//    for(int row = t->length - 1; row >= 0; row--)
//    {
//#ifdef SHOWTAB
//        printf("row %d with cols,| len = %d ?= colc %d\n", row, t->rows_v[row].length, t->rows_v[row].cols_c );
//#endif
//        for(int col = t->rows_v[row].length - 1; col >= 0; col--)
//        {
//#ifdef SHOWTAB
//            printf(" %d", col);
//#endif
//            FREE(t->rows_v[row].cols_v[col].elems_v)
//        }
//        FREE(t->rows_v[row].cols_v)
//#ifdef SHOWTAB
//        printf(" freed\n");
//#endif
//    }
//    FREE(t->rows_v)
//#ifdef SHOWTAB
//    printf("table freed\n");
//#endif
//}

#ifdef CMDS

char *print_opt(int c)
{
    if(c == SET)
    {return "-set-";}
    else if(c == FIND)
    {return "-find-";}
    else if(c == MIN)
    {return "-min-";}
    else if(c == MAX)
    {return "-max-";}
    else if(c == CLEAR)
    {return "-clear-";}
    else if(c == IROW)
    {return "-irow-";}
    else if(c == AROW)
    {return "-arow-";}
    else if(c == DROW)
    {return "-drow-";}
    else if(c == ICOL)
    {return "-icol-";}
    else if(c == ACOL)
    {return "-acol-";}
    else if(c == DCOL)
    {return "-dcol-";}
    else return "-NaN-";
}

char* print_cmd_opt(int c)
{
    switch (c)
    {
        case NOPT:
            return "-NOPT-";
        case PRC:
            return "-PRC-";
        case SEL:
            return "-SEL-";
        default:
            return "-NaN-";
    }
}

#endif

/* frees all memory allocated by a table_t structure s */
void free_table(tab_t *t)
{

#ifdef SHOWTAB
    printf("line %d FREE TABLE Table :len %d ?= rows %d\n", __LINE__, t->length, t->row_c);
#endif
    for(int row = t->length - 1; row >= 0; row--)
    {
#ifdef SHOWTAB
        printf("row %d with cols,| len = %d ?= colc %d\n", row, t->rows_v[row].length, t->rows_v[row].cols_c);
#endif
        for(int col = t->col_c - 1; col >= 0; col--) // FIXME (1) + 1
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

/* frees all memory allocated by a cl_t structure s */
void free_cl(cl_t *s)
{
    FREE(s->seps.elems_v)
    if(s->ptr)
        fclose(s->ptr);
}

void clear_data(tab_t *t, cl_t *cl)
{
    free_table(t);
    free_cl(cl);
}
//endregion

//region CONSTRUCTORS

void carr_clear(carr_t *carr)
{
#ifdef CMDS
    printf("\nCARR_CLEAR\n(%d) bef \"%s\" ", __LINE__, carr->elems_v);
#endif
    carr->isempty = true;
    memset(carr->elems_v, 0, carr->elems_c); // FIXME fix me :) dont use memset
    carr->elems_c = 0;
#ifdef CMDS
    printf("aft \"%s\"\n", carr->elems_v);
#endif
}


void row_clear(row_t *row)
{
#ifdef CMDS
    printf("\n(%d) row_clear cols_c(%d) len(%d)\n", __LINE__, row->cols_c, row->length);
#endif
    for(int i = 0; i < row->cols_c + 1; i++) // FIXME memory
    {
        carr_clear(&row->cols_v[i]);
    }
}

void carr_ctor(carr_t *arr, int size, int *exit_code)
{
    arr->elems_v = (char*)calloc(size, sizeof(char));
    CHECK_ALLOC_ERR(arr->elems_v)

    arr->length  = size;
    arr->elems_c = 0;
    arr->isempty = true;
    arr->isnum   = false;
}

/* allocating a row of size siz */
void row_ctor(row_t *r, int size, int *exit_code)
{
    r->cols_v = (carr_t*)malloc(size * sizeof(carr_t));
    CHECK_ALLOC_ERR(r->cols_v)

    /* allocate cols */
    for(int cols = 0; cols < size; cols++)
    {
        carr_ctor(&r->cols_v[cols], MEMBLOCK, exit_code);
        CHECK_EXIT_CODE
    }
    r->length = size;
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
        row_ctor(&t->rows_v[row], MEMBLOCK, exit_code);
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
 * Copies string src to string string cmd
 *
 * @param array of chars to copy to
 * @param array of chars to copy from
 * @return false If len of cmd < len of src
 *         true if copied succesfully
 */
bool strcop(char* cmd, char* src)
{
    int cmdlen = (int)strlen(cmd);
    if(cmdlen < (int)strlen(src))
    {
#ifdef CMDS
    printf("(%d) cmd_len(%d) != srclen(%d) for cmd=\"%s\",  src=\"%s\"\n",__LINE__, cmdlen, (int)strlen(src),cmd, src );
#endif
        return false;
    }

    int i = 0;
    while(i < cmdlen)
    {
        cmd[i] = src[i];
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
    arr->elems_v[arr->elems_c]   = 0; /* add terminating 0 */

}

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

//region TABLE RESIZE

/**
 *  Expand the table
 *
 * @param cl
 * @param t
 * @param exit_code if memore was not allocated
 */
void expand_tab(cl_t *cl, tab_t *t, int *exit_code) // FIXME
{
    int sel_pos = cl->currsel;
    /* expand_rows */
    if(cl->cmds[sel_pos].row_2 - 1 > t->row_c)
    {
#ifdef SELECT
        printf("(%d) row_c %d increased to %d\n", __LINE__, t->row_c, cl->cmds[sel_pos].col_2);
#endif
        /* allocate new memory for rows */
        t->length = cl->cmds[sel_pos].col_2;
        t->rows_v = (row_t*)realloc(t->rows_v, t->length * sizeof(row_t));
        CHECK_ALLOC_ERR(t->rows_v)


        /* allocate new memory for columns */
        int siz = (cl->cmds[sel_pos].col_2 == MAXVAL) ? t->col_c : cl->cmds[sel_pos].col_2;

        for(int r = t->row_c + 1; r < t->length; r++) // FIXME убрать + 1
        {
            row_ctor(&t->rows_v[r], siz, exit_code);
            CHECK_EXIT_CODE
        }
        t->row_c = t->length - 1;
    }

    /* expand collumns */
    int to = (cl->cmds[cl->currsel].row_2 == MAXVAL) ? t->length : cl->cmds[cl->currsel].row_2;

    for(int r = cl->cmds[cl->currsel].row_1 - 1; r < to; r++)
    {
        if(cl->cmds[cl->currsel].col_2 - 1 > t->rows_v[r].cols_c)
        {
            t->rows_v[r].length  = cl->cmds[cl->currsel].col_2;
            t->rows_v[r].cols_v  = (carr_t*)realloc(t->rows_v[r].cols_v, t->rows_v[r].length * sizeof(carr_t));
            CHECK_ALLOC_ERR(t->rows_v[r].cols_v)

            for(int c = t->rows_v[r].cols_c + 1; c < t->rows_v[r].length; c++)
            {
                carr_ctor(&t->rows_v[r].cols_v[c], MEMBLOCK, exit_code);
                CHECK_EXIT_CODE
            }
            t->rows_v[r].cols_c = t->rows_v[r].length - 1;
        }
    }
}

/* trims an array of characters */
void cell_trim(carr_t *arr, int *exit_code)
{
    if(!arr->elems_v)
    {
        printf("\t\t\t шынгыс: ты чо охуел куда ты лезеш\n");
        arr->elems_v = (char*)calloc(MEMBLOCK ,sizeof(char));
        exit(228);
    }

    if(arr->length >= arr->elems_c + 1)
    {
#ifdef SHOWTAB
        printf("(%d)\t\t\tlen %d elems %d\n", __LINE__, arr->length, arr->elems_c); // FIXME what the fuck
#endif
        /* reallocate for elements and terminating 0 */
        arr->elems_v = (char *)realloc(arr->elems_v, (arr->elems_c + 1) * sizeof(char));
        CHECK_ALLOC_ERR(arr->elems_v)

        arr->length = (arr->elems_c) ? arr->elems_c + 1 : 1; // FIXME valgriind throw out + 1
    }
}

void row_trim(row_t *row, int siz, int *exit_code, rtrim_opt opt)
{
    /* trim all cells in the row */
#ifdef SHOWTAB
    printf("(%d) cols -> %d  len -> %d . \n", __LINE__, row->cols_c, row->length );
#endif

    if(row->length >= siz)
    {
        for(int i = row->length - 1; i >= siz; i--)
        {
            FREE(row->cols_v[i].elems_v)
        }

        row->cols_v = (carr_t *)realloc(row->cols_v, siz * sizeof(carr_t));
        CHECK_ALLOC_ERR(row->cols_v)
        row->length = siz;
        row->cols_c = siz - 1;
    }
    else if(row->length < siz)
    {
        row->cols_v = (carr_t *)realloc(row->cols_v, siz * sizeof(carr_t));
        CHECK_ALLOC_ERR(row->cols_v)

        for(int c = row->cols_c + 1; c < siz; c++)
        {
            carr_ctor(&row->cols_v[c], MEMBLOCK, exit_code);
            CHECK_EXIT_CODE
        }

        row->length = siz;
        row->cols_c = siz - 1;
    }

    if(opt == WCOLS)
    {
        for(int cell = row->cols_c; cell >= 0; cell--)
        {
            cell_trim(&row->cols_v[cell], exit_code);
            CHECK_EXIT_CODE
        }
    }


#ifdef SHOWTAB
    printf("(%d) len %d cols %d \n", __LINE__, row->length, row->cols_c);
#endif
}

//void row_trim_1(row_t *row, int newsize, int *exit_code)
//{
//    /* trim all cells in the row */
//#ifdef SHOWTAB
//    printf("(%s) cols -> %d  len -> %d . \n", __FUNCTION__, row->cols_c, row->length);
//#endif
//    for(int cell = row->cols_c; cell >= 0; cell--)
//    {
//        cell_trim(&row->cols_v[row->cols_c], exit_code);
//        CHECK_EXIT_CODE
//    }
//
//    for(int i = row->length - 1; i > row->cols_c; i--)
//    {FREE(row->cols_v[i].elems_v)}
//
//    //TODO check if there is a need to trim a row
//    row->cols_v = (carr_t *)realloc(row->cols_v, (row->cols_c + 1) * sizeof(carr_t));
//    CHECK_ALLOC_ERR(row->cols_v)
//
//    row->length = row->cols_c + 1;
//#ifdef SHOWTAB
//    printf("(%d) %s: len %d cols %d \n", __LINE__, __FUNCTION__);
//#endif
//}

/* trims a table by reallocating rows and cols */
void table_trim(tab_t *t, int *exit_code)
{
    int row;
    int maxlen = 0;
#ifdef SHOWTAB
    printf("\nTABLE_TRIM\n(%d) rows(%d)",__LINE__);
#endif

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
    for(row = 0; row < t->row_c; row++) // FIXME память
    {
#ifdef SHOWTAB
        printf("\n(%d) untrim row %d has len %d\n", __LINE__,row, t->rows_v[row].length);
#endif
        row_trim(&t->rows_v[row], t->rows_v[row].cols_c + 1, exit_code, WCOLS); //FIXME
        CHECK_EXIT_CODE

        if(!row || maxlen < t->rows_v[row].cols_c) // find max number of cells
        {
            maxlen = t->rows_v[row].cols_c + 1;
        }
#ifdef SHOWTAB
        printf("(%d)     trim row %d has len %d\n", __LINE__, row, t->rows_v[row].length);
#endif
    }

#ifdef SHOWTAB
    printf("\n(%d)t->row_c = %d \n\n",__LINE__, t->row_c);
#endif
    for(int r = t->row_c - 1; r >= 0; r--) //FIXME cols - 1
    {
#ifdef SHOWTAB
        printf("(%d) maxlen %d untrim row %d has len %d\n", __LINE__, maxlen, r, t->rows_v[r].length);
#endif
        row_trim(&t->rows_v[r], maxlen, exit_code, NCOLS);
#ifdef SHOWTAB
        printf("(%d)     maxlen %d trim row %d has len %d\n", __LINE__, maxlen, r, t->rows_v[r].length);
#endif
    }

    t->col_c = maxlen; //FIXME (1) -1
}
//endregion

//region GETTERS

void set_cursel(cl_t *cl, int row1, int col1, int row2, int col2, int *exit_code)
{
    if(lessthan(row2, row1) || lessthan(col2, col1) ||
        lessthan(row1, cl->cmds[cl->cellsel].row_1) ||
        lessthan(col1, cl->cmds[cl->cellsel].col_1) ||
        lessthan(row2, cl->cmds[cl->cellsel].row_2) ||
        lessthan(col2, cl->cmds[cl->cellsel].col_2))
    {
        *exit_code = W_SEPARATORS_ERR;
        CHECK_EXIT_CODE
    }

#ifdef SELECT
    printf("(\nSET_CURSEL\n%d) cursel changed: from [%d,%d,%d,%d] to [%d,%d,%d,%d]\n", __LINE__, cl->cmds[cl->cellsel]
    .row_1,
           cl->cmds[cl->cellsel].col_1, cl->cmds[cl->cellsel].row_2, cl->cmds[cl->cellsel].col_2, row1, col1, row2,
           col2);
#endif
    cl->currsel = cl->cmds_c;
    cl->cmds[cl->currsel].row_1 = row1;
    cl->cmds[cl->currsel].col_1 = col1;
    cl->cmds[cl->currsel].row_2 = row2;
    cl->cmds[cl->currsel].col_2 = col2;
}

/* sets new cursel to the column with min/max numerical value from the cursel */
void getmcol(tab_t *t, cl_t *cl, int *exit_code) // TODO
{
    return;
    int row_fr = cl->cmds[cl->currsel].row_1 - 1;

    int row_to = (cl->cmds[cl->currsel].row_2 == MAXVAL)
            ? t->row_c
            : cl->cmds[cl->currsel].row_2 - 1;

    int col_fr = 0, col_to = 0;
    int m_row  = 0, m_col  = 0; /* initialize positions */

    double m_val   = 0; /* if there will be no numbers change nothing */
    double tmp_val = 0;

    bool set = false;

    for( ;row_fr <= row_to; row_fr++)
    {
        col_fr = cl->cmds[cl->currsel].col_1 - 1;
        col_to = (cl->cmds[cl->currsel].col_2 == MAXVAL)
                ? t->rows_v[row_fr].cols_c
                : cl->cmds[cl->currsel].col_2 - 1;
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
                else if(cl->cmds[cl->currsel].cmd_opt == MIN && tmp_val < m_val)
                {
#ifdef SELECT
                    printf("(%d)        min m_val %lf m_row = %d m_col = %d\n", __LINE__, m_val, m_row, m_col);
#endif
                    m_val = tmp_val;
                    m_row = row_fr, m_col = col_fr;
                }
                else if(cl->cmds[cl->currsel].cmd_opt == MAX && tmp_val > m_val)
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
        set_cursel(cl, m_col, m_col, m_row, m_col, exit_code);
        CHECK_EXIT_CODE
    }
}

/* returns true is newline or eof reached */
void get_nums(carr_t *cmd, cl_t *cl, int *exit_code) // TODO поменять иусловия для cursel
{
    char *token = NULL;
    char *ptr   = NULL;
    int nums[4] = {0};
    int i       = 0;
    char uscore = 0;
    char dash   = 0;

#ifdef SELECT
    printf("\nGET_NUMS\n(%d) cmd = \"%s\"\n",__LINE__, cmd->elems_v);
#endif
    /* get the first token */
    token = strtok(cmd->elems_v, ",");

    /* walk through other tokens */
    for(; i < 4 && token != NULL; i++)
    {
        /* add a number to an array */
        nums[i] = (int)(strtol(token, &ptr, 10));

        if(nums[i] <= 0)
        {
            if(!strcmp(ptr, "-"))
            {
                if(++dash == 3)
                {
                    *exit_code = VAL_UNSUPARG_ERR;
                }
                nums[i] = MAXVAL;
            }
            else if(!strcmp(ptr, "_"))
            {
                uscore++;
                nums[i] = ALLVALS;
            }
            else
            {
                *exit_code = W_SEPARATORS_ERR;
            }
            CHECK_EXIT_CODE
        }
        token = strtok(NULL, ",");
    }
    FREE(token) // FIXME check if there is a need to call this function

    /* if entered selection looks like [R1,C1] */
    if(i == 2)
    {
        nums[2] = nums[0];
        nums[3] = nums[1];
        switch(uscore)
        {
            case 0:
                cl->cmds[cl->cmds_c].sel_opt = CELL;
                cl->cellsel = cl->cmds_c;
                break;
            case 1:
                cl->cmds[cl->cmds_c].sel_opt = CELLS;
                break;
            case 2:
                cl->cmds[cl->cmds_c].sel_opt = WHOLE;
                break;
            default:
                *exit_code = UNEXPD_BEHAV_ERR;
                CHECK_EXIT_CODE
        }
    }

    /* if entered selection looks like [R1,C1,R2,C2] */
    else if(i == 4)
    {
        if(nums[2] == nums[0] && nums[3] == nums[1])
        {
            cl->cmds[cl->cmds_c].sel_opt = CELL;
        }
        else cl->cmds[cl->cmds_c].sel_opt = CELLS;
    }
    else
    {
        *exit_code = NUM_UNRECARG_ERR;
        CHECK_EXIT_CODE
    }
#ifdef SELECT
    printf("(%d) extracted selection: [%d,%d,%d,%d]\n",__LINE__, nums[0], nums[1], nums[2], nums[3]);
#endif

    set_cursel(cl, nums[0], nums[1], nums[2], nums[3], exit_code);
    CHECK_EXIT_CODE
}

bool get_cell(carr_t *col, cl_t *cl, int *exit_code)
{
    char buff_c;
    bool quoted = false;
    for(col->elems_c = 0; ;)
    {

        buff_c = (char)fgetc(cl->ptr);
        if(buff_c == '\n' || feof(cl->ptr))
        {
            if(quoted) *exit_code = Q_DONT_MATCH_ERR;
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
        else if(strchr(cl->seps.elems_v, buff_c) != NULL && !quoted)
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
void get_row(row_t *row, cl_t *cl, int *exit_code)
{
    bool end;
    for(row->cols_c = 0; !feof(cl->ptr); row->cols_c++ )
    {
        if(row->cols_c == row->length)
        {
            row->length += MEMBLOCK;
            row->cols_v = (carr_t *)realloc(row->cols_v, row->length * sizeof(carr_t));
            CHECK_ALLOC_ERR(row->cols_v)

            /* create a col */
            carr_ctor(&row->cols_v[row->cols_c], MEMBLOCK, exit_code);
            CHECK_EXIT_CODE
        }
#ifdef SHOWTAB
        printf("(%d)  \t\tcol %d\n", __LINE__, row->cols_c);
#endif
        end = get_cell(&row->cols_v[row->cols_c], cl, exit_code);
        CHECK_EXIT_CODE

        /* check if the cell is a number and set true or false to the cell */
        iscellnum(&row->cols_v[row->cols_c]);

        if(end)
        {
#ifdef SHOWTAB
            printf("(%d)  \tlen %d\n", __LINE__, row->length);
#endif
            return;
        }
    }
}

void get_table(const int *argc, const char **argv, tab_t *t, cl_t *cl, int *exit_code)
{
    //TODO check if file is not empty
    if((cl->ptr = fopen(argv[*argc - 1], "r+")) == NULL)
    {
        *exit_code = NO_SUCH_FILE_ERR;
        return;
    }

    /* if file has been opened successfully */
    else
    {
        /* allocate a table */
        table_ctor(t, exit_code);
        CHECK_EXIT_CODE

        /* write the table to the structure */
        for(t->row_c = 0; !feof(cl->ptr); t->row_c++ )
        {
            /* realloc the table to the new size */
            if(t->row_c == t->length)
            {
                t->length += MEMBLOCK;
                t->rows_v  = (row_t*)realloc(t->rows_v, t->length * sizeof(row_t));
                CHECK_ALLOC_ERR(t->rows_v)
                /* create a row */
                row_ctor(&t->rows_v[t->row_c],MEMBLOCK, exit_code);
            }
#ifdef SHOWTAB
            printf("(%d)  row %d\n", __LINE__, t->row_c);
#endif
            get_row(&t->rows_v[t->row_c], cl, exit_code);
            CHECK_EXIT_CODE
        }
        /* align a table */
        table_trim(t, exit_code);
        CHECK_EXIT_CODE
    }
}
//endregion

//region functions

// TODO документация
void clear_f(cl_t *cl, tab_t *t)
{
    int row_fr = cl->cmds[cl->currsel].row_1;
    int col_fr = cl->cmds[cl->currsel].col_1;
#ifdef CMDS
    printf("\nCLEAR_F\n(%d) [%d,%d,%d,%d]\n",__LINE__, row_fr, col_fr,
            cl->cmds[cl->currsel].row_2, cl->cmds[cl->currsel].col_2);
#endif

    for( ; row_fr < cl->cmds[cl->currsel].row_2; row_fr++)
    {
        for( ; col_fr < cl->cmds[cl->currsel].col_2; col_fr++)
        {
            carr_clear(&t->rows_v[row_fr].cols_v[col_fr]);
        }
    }
}

// TODO документация
void irow_f(cl_t *cl, tab_t *t, int *exit_code)
{
    int row_fr = (cl->cmds[cl->currsel].row_1 == ALLVALS) ? 0 : cl->cmds[cl->currsel].row_1 - 1;
#ifdef CMDS
    printf("(%d) irow row_fr(%d) col_c(%d) row_c(%d) len(%d)", __LINE__, row_fr, t->col_c, t->row_c, t->length);
#endif
    /* allocate memory for a new row */
    t->rows_v = (row_t*)realloc(t->rows_v, (++t->length) * sizeof(row_t));
    CHECK_ALLOC_ERR(t->rows_v)
    t->row_c++;

    /* move all rows */
    for(int i = t->row_c - 1; i >= row_fr; i--)
    {
        t->rows_v[i+1] = t->rows_v[i];
    }

    /* create a row */
    row_ctor(&t->rows_v[row_fr],t->col_c, exit_code);
}


void arow_f(cl_t *cl, tab_t *t, int *exit_code)
{
    (void)cl;
    (void)t;
    (void)exit_code;
}


void drow_f(cl_t *cl, tab_t *t, int *exit_code)
{
    (void)cl;
    (void)t;
    (void)exit_code;
}


void icol_f(cl_t *cl, tab_t *t, int *exit_code)
{
    (void)cl;
    (void)t;
    (void)exit_code;
}


void acol_f(cl_t *cl, tab_t *t, int *exit_code)
{
    (void)cl;
    (void)t;
    (void)exit_code;
}


void dcol_f(cl_t *cl, tab_t *t, int *exit_code)
{
    (void)cl;
    (void)t;
    (void)exit_code;
}
//endregion

char* print_selopt(int x)
{
    switch(x)
    {
        case WHOLE:
            return "-whole-";
        case CELL:
            return "-cell-";
        case CELLS:
            return "-cells-";
        case TMP:
            return "TMP";
        default:
            return "-undef-";
    }
}

// TODO написать коммантарий
void proc_sel(cl_t *cl, tab_t *t)
{
#ifdef CMDS
    printf("\nPROC_SEL\n(%d), for %s bef [%d,%d,%d,%d] ", __LINE__,print_selopt(cl->cmds[cl->currsel].sel_opt), cl->cmds[cl->currsel].row_1,
           cl->cmds[cl->currsel].col_1,
           cl->cmds[cl->currsel].row_2,
           cl->cmds[cl->currsel].col_2);
#endif
    if(!cl->cmds[cl->currsel].isset)
    {
        switch(cl->cmds[cl->currsel].sel_opt)
        {
            case WHOLE:
                cl->cmds[cl->currsel].row_1 = 0;
                cl->cmds[cl->currsel].row_2 = t->row_c;//FIXME + 1
                cl->cmds[cl->currsel].col_1 = 0;
                cl->cmds[cl->currsel].col_2 = t->col_c;
                break;
            case CELL:
                if(cl->cmds[cl->currsel].row_1 == ALLVALS)
                {
                    cl->cmds[cl->currsel].row_1 = 0;
                    cl->cmds[cl->currsel].row_2 = t->row_c;//FIXME + 1
                }
                if(cl->cmds[cl->currsel].col_1 == ALLVALS)
                {
                    cl->cmds[cl->currsel].col_1 = 0;
                    cl->cmds[cl->currsel].col_2 = t->col_c;
                }
                break;
            case CELLS:
                if(cl->cmds[cl->currsel].row_2 == MAXVAL)
                {
                    cl->cmds[cl->currsel].row_2 = t->row_c;//FIXME + 1
                }else if(cl->cmds[cl->currsel].row_2 == ALLVALS)
                {
                    cl->cmds[cl->currsel].row_1 = 0;
                    cl->cmds[cl->currsel].row_2 = t->row_c;//FIXME + 1
                }

                if(cl->cmds[cl->currsel].col_2 == MAXVAL)
                {
                    cl->cmds[cl->currsel].col_2 = t->col_c - 1;
                }else if(cl->cmds[cl->currsel].col_2 == ALLVALS)
                {
                    cl->cmds[cl->currsel].col_1 = 0;
                    cl->cmds[cl->currsel].col_2 = t->col_c - 1;
                }
                break;
            case TMP:
                printf("(%d) TMP SHEET\n",__LINE__);
                break;
        }
        cl->cmds[cl->currsel].isset = true;
#ifdef CMDS
        printf("aft [%d,%d,%d,%d]\n", cl->cmds[cl->currsel].row_1,
               cl->cmds[cl->currsel].col_1,
               cl->cmds[cl->currsel].row_2,
               cl->cmds[cl->currsel].col_2);
#endif
    }
}

//region TABLE PROCESSING
void process_table(cl_t *cl, tab_t *t, int *exit_code)
{
#ifdef CMDS
    printf("(%d)opt %s for cmd_c %d\n\n ",__LINE__, print_opt(cl->cmds[cl->cmds_c].proc_opt), cl->cmds_c);
#endif

    switch(cl->cmds[cl->cmds_c].proc_opt)
    {
        case FIND: { /* find_f(cl, t, exit_code); */ break;}// TODO

        /* set column in range of the selection by numerical value */
        case MIN:{ getmcol(t, cl, exit_code); break; }

        /* set column in range of the selection by numerical value */
        case MAX:{ getmcol(t, cl, exit_code); break; }

        case CLEAR: { clear_f(cl, t);  break;}//TODO
        case IROW:  { irow_f(cl, t, exit_code);  break;} //TODO
        case AROW:  { arow_f(cl, t, exit_code);  break;} //TODO
        case DROW:  { drow_f(cl, t, exit_code);  break;} //TODO
        case ICOL:  { icol_f(cl, t, exit_code);  break;} //TODO
        case ACOL:  { acol_f(cl, t, exit_code);  break;} //TODO
        case DCOL:  { dcol_f(cl, t, exit_code);  break;} //TODO
        default:    {break;}
    }
}
//endregion

//region cmdLINE ARGS PARSING

/**
 *
 * @param cmd
 * @param cl
 * @param t
 * @param exit_code
 */
void init_wspased_cmd(carr_t *cmd, cl_t *cl, tab_t *t, int *exit_code)
{

    return;
    if(strbstr(cmd->elems_v, "[find "))
    {

        /* if cmd is FIND and a pattern is too long */
        if(!strcop(cl->cmds[cl->cmds_c].pttrn, cmd->elems_v))
        {
            *exit_code = LEN_UNSUPCMD_ERR;
            error_line_global = __LINE__;
        }
#ifdef SELECT
        printf("(%d) cmd find, pattern \"%s\"\n", __LINE__, cl->cmds[cl->cmds_c].pttrn );
#endif
        return;
    }
}

/**
 *  Initialize a cmd from argument
 *  If the given cmd is a selection,
 *   then change the current selection to the new given selection
 * @param cmd an array with a cmd
 * @param cl structure with cmdline aruments, including an array
 *        with cmds and current selection
 * @param t structure with a table to call table editing functions
 * @param exit_code
 */
void init_n_wspased_cmd(carr_t *cmd, cl_t *cl, int *exit_code)
{
    /* number of occurances */
    char *currcom = calloc(cmd->elems_c, sizeof(char));
    CHECK_ALLOC_ERR(currcom)


    if     (!strcmp(cmd->elems_v, "irow" )) { cl->cmds[cl->cmds_c].proc_opt = IROW; }
    else if(!strcmp(cmd->elems_v, "arow" )) { cl->cmds[cl->cmds_c].proc_opt = AROW; }
    else if(!strcmp(cmd->elems_v, "drow" )) { cl->cmds[cl->cmds_c].proc_opt = DROW; }
    else if(!strcmp(cmd->elems_v, "icol ")) { cl->cmds[cl->cmds_c].proc_opt = ICOL; }
    else if(!strcmp(cmd->elems_v, "acol ")) { cl->cmds[cl->cmds_c].proc_opt = ACOL; }
    else if(!strcmp(cmd->elems_v, "dcol" )) { cl->cmds[cl->cmds_c].proc_opt = DCOL; }
    else if(!strcmp(cmd->elems_v, "clear")) { cl->cmds[cl->cmds_c].proc_opt = CLEAR;}
    else if(!strcmp(cmd->elems_v, "min"  )) { cl->cmds[cl->cmds_c].proc_opt = MIN;  }
    else if(!strcmp(cmd->elems_v, "max"  )) { cl->cmds[cl->cmds_c].proc_opt = MAX;  }
    else if(!strcmp(cmd->elems_v, "set"  )) { cl->cmds[cl->cmds_c].proc_opt = SET;  }// TODO разберись в set
    else if(!strcmp(cmd->elems_v, "_"    )) // TODO обновит выбор с временной переменной
    {
        printf("%d _", __LINE__);
    } // TODO refresh selection from temp var

        /* if there's a selection cmd sets a new current selection */
    else
    {
        get_nums(cmd, cl, exit_code);
        cl->cmds[cl->cmds_c].cmd_opt = SEL;
        cl->cmds[cl->cmds_c].proc_opt = NOPROC;
    }

    if(cl->cmds[cl->cmds_c].proc_opt >= MIN && cl->cmds[cl->cmds_c].proc_opt <= DCOL)
    {
        cl->cmds[cl->cmds_c].cmd_opt = PRC; /* now cmd option is processing the data */
        cl->cmds[cl->cmds_c].sel_opt = NOSEL;
    }

    CHECK_EXIT_CODE
    FREE(currcom)
}

void a_ch_cmd(carr_t *cmd, char item, bool quoted, int *exit_code)
{
    if(item == '[' || item == ']' || item == ';') //TODO change me to a function
    {
        if(quoted)
        {a_carr(cmd, item, exit_code);}
        else
        {return;}
    } else
    {a_carr(cmd, item, exit_code);}
}

//TODO написать документацию, // FIXME хзхз
void prep_for_next_cmd(carr_t *cmd, int *cmds_c, const int *pos, const int *arglen )
{
    /* move to the next cmd */
    if(*pos != *arglen - 1)
        (*cmds_c)++;

    carr_clear(cmd);
}


/* Pocess cmdm, check if the cmd have ben initialized and call function to process the table or change an
 * exit_code */ // TODO написать нормаьную документацию
void process_cmd(cl_t *cl, tab_t *t, int *exit_code)
{
#ifdef CMDS
    printf("\nPROCESS_CMD \n(%d)command opt = %s \n",
            __LINE__, print_cmd_opt(cl->cmds[cl->cmds_c].cmd_opt));
#endif
    switch(cl->cmds[cl->cmds_c].cmd_opt)
    {
        case SEL: /* selection is already set in the functinon */
            cl->cmds[cl->currsel].isset = false;

            /* change selection to fit the table, optionally expend the table *//* expand tab after a comamnd */ // TODO move to the fun
            proc_sel(cl,t);

            /* expand tab to fit the selection */
            expand_tab(cl, t, exit_code);

#ifdef CMDS
            printf("(%d) cursel: [%d,%d,%d,%d]\n", __LINE__, cl->cmds[cl->currsel].row_1,
                   cl->cmds[cl->currsel].col_1,
                   cl->cmds[cl->currsel].row_2,
                   cl->cmds[cl->currsel].col_2
            );
#endif
            break;
        case PRC: /* call a function initialized by a given parameter */
#ifdef CMDS
            printf("(%d) fun = %s\n", __LINE__, print_opt(cl->cmds[cl->cmds_c].proc_opt));
#endif
            process_table(cl, t, exit_code);
            break;
        case NOPT:
            *exit_code = NUM_UNRECARG_ERR;
            CHECK_EXIT_CODE
    }
}

void init_cmd(carr_t *cmd, tab_t *t, cl_t *cl, int *exit_code)
{
    cell_trim(cmd, exit_code);
    CHECK_EXIT_CODE
#ifdef SELECT
    printf("\nINIT_CMD\n(%d) cmd = \"%s\"\n", __LINE__, cmd->elems_v);
#endif

    if(strlen(cmd->elems_v) > 1)
    {
        /* edit structure of the table or change current selection */
        if(strchr(cmd->elems_v, ' ') == NULL)
        {
            init_n_wspased_cmd(cmd, cl, exit_code);
        }
        else /* init and call data processing functions or process temp variables */
        {
            init_wspased_cmd(cmd, cl, t, exit_code);
        }
        CHECK_EXIT_CODE
    }
    else
    {
        *exit_code = LEN_UNSUPCMD_ERR;
        CHECK_EXIT_CODE
    }
}

/**
 *  For each agrument (selection or processing cmd)
 *  copies everything before ; or before \0 t to the temp array,
 *  then parses the temp array and calls the function optionally
 *
 * @param cmd ar array where to copy a cmd
 * @param src a cmdline argument
 * @param exit_code can be changed if the error occured
 */
void init_cmds(carr_t *cmd, const char *arg, cl_t *cl, tab_t *t, int *exit_code)
{
#ifdef CMDS
    printf("\n\n(%d) init_cmds: \n", __LINE__);
#endif
    //region variables
    int arglen = (int)strlen(arg);
    bool quoted = false;

    /* firsst cell selection command is on the position 0,
     * also it is neccesarry to init cmds count */
    cl->cellsel = (cl->currsel = (cl->cmds_c = 0));

    /* current command(0) is a selection */
    cl->cmds[cl->cmds_c].cmd_opt = SEL;

    /* set a selection to the first cell(by default) */
    cl->cmds[cl->currsel].row_1 = 1;
    cl->cmds[cl->currsel].col_1 = 1;
    cl->cmds[cl->currsel].row_2 = 1;
    cl->cmds[cl->currsel].col_2 = 1;

    /* first cmd is a 1 1(default) selection  */
    cl->cmds_c = 1;
    //endregion

    /* walk through other tokens */
    for(int p = 0; p < arglen; p++)
    {
        if(arg[p] == '\"')
        {
            NEG(quoted)
        }

        /* if the end of the argument reached */
        else if((arg[p] == ';' && !quoted) || p == arglen - 1)// FIXME
        {
            if(p == arglen - 1 || arg[p] != ';')
            {
                a_ch_cmd(cmd, arg[p], quoted, exit_code);
            }
            /* initialize a cmd */
            init_cmd(cmd, t, cl, exit_code);

            /* process an initialized cmd change the exit_code */
            process_cmd(cl,t,exit_code);
            CHECK_EXIT_CODE

            prep_for_next_cmd(cmd, &cl->cmds_c, &p, &arglen);
#ifdef CMDS
            printf("(%d) prepared: next cmd(%d), arglen(%d), pos(%d)\n", __LINE__, cl->cmds_c, arglen, p);
#endif
        }
        /* add a char to the cmd arr */
        a_ch_cmd(cmd, arg[p],quoted,exit_code);
        CHECK_EXIT_CODE

    }
}

/**
 * Initializes an array of entered separators(DELIM)
 *
 * @param argc Number of cmdline arguments
 * @param argv An array with cmdline arguments
 * @param exit_code exit code to change if an error occurred
 */
void init_separators(const int *argc, const char **argv, cl_t *cl, int *exit_code)
{
    //region variables
    cl->seps.length = 0;
    carr_ctor(&cl->seps, MEMBLOCK, exit_code);
    CHECK_EXIT_CODE
    int k = 0;
    //endregion

    /* if there is only name, functions and filename */
    if(*argc == 3 && strcmp(argv[1], "-d") != 0)
    {
        a_carr(&cl->seps, ' ', exit_code);
        cl->defaultsep = true;
        CHECK_EXIT_CODE
    }

        /* if user entered -d flag and there are function names and filename in the cmdline */
    else if(*argc == 5 && !strcmp(argv[1], "-d"))
    {
        cl->defaultsep = false;
        while(argv[2][k])
        {
            /* checks if the user has not entered characters that will lead to undefined program behavior */
            if( argv[2][k] == '\\' || argv[2][k] == '\"' || argv[2][k] == '\'')
            {
                *exit_code = W_SEPARATORS_ERR;
                CHECK_EXIT_CODE
            }
            set_add_item(&(cl->seps), argv[2][k], exit_code);
            CHECK_EXIT_CODE
            k++;
        }
    }

    else
    {
        *exit_code = W_SEPARATORS_ERR;
        CHECK_EXIT_CODE
    }
#ifdef SEPSBUG
    printf("line %d separators -> ", __LINE__);
    for(int i = 0; i < cl->seps.elems_c; i++ )
        printf("%c",cl->seps.elems_v[i]);
    putchar(10);
#endif
}
//endregion

/** TODO add the ability to extract cmds from a file
 * calls a function to parse cmdline arguments
 */
void parse_cl_proc_tab(const int *argc, const char **argv, tab_t *t, cl_t *cl, int *exit_code)
{
    /* by default has value 1, which means there was no delim in the cmd line */
    int clarg = (cl->defaultsep) ? POSNDEL : POSWDEL;

    carr_t cmd;      /* an array for one cmd from the set fo the cmds */
    carr_ctor(&cmd, MEMBLOCK, exit_code);
    CHECK_EXIT_CODE

    /* means there is no cmds entered in the cmd line */
    if(clarg == *argc - 1)
    {
        *exit_code = VAL_UNSUPARG_ERR;
        error_line_global = __LINE__;
        return;
    }

    /* copy an argument to the array and call the function for it */
    init_cmds(&cmd, argv[clarg], cl, t, exit_code);
    CHECK_EXIT_CODE
    FREE(cmd.elems_v)
}

void print_error_message(const int *exit_code)
{
    /* an array with all error messages */
    char *error_msg[] =
    {
            "Twenty afternoons in utopia... Cannot allocate memory",
            "Entered separators are not supported by the program",
            "You've entered wrong number of arguments",
            "cmd you've entered has unsupported value",
            "There is no file with this name",
            "cmd you've entered is too long",
            "Quotes dont match",

            "Why?! And we light up the sky!!!",
            "Watch me die",
            "Wake me up yesterdays morning",
            "Choke me by a spagetti"
    };

    /* print an error message to stderr */
    fprintf(stderr, "Error %d: %s occured on the line %d.\n", *exit_code, error_msg[*exit_code - 1], error_line_global);
}

/*
 * Runs the program: initializes structures, then initializes cmdline arguments, then processes the table
 * After every initializing checks for an exit_code to return it if the error has been occurred
 * Frees memory that was allocated while program was running.
 */
int run_program(const int argc, const char **argv)
{
    //region arguments
    int exit_code = 0;
    tab_t  t;
    cl_t   cl;
    //endregion

    /* initialize separators from cmdline */
    init_separators(&argc, argv, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* adds table to the structure */
    get_table(&argc, argv, &t, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* parse cmdline arguments and process table for them */
    parse_cl_proc_tab(&argc, argv, &t, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* trim table before printing */ // TODO добавить арг final, который значит, что надо все подровнять
    //table_trim_bef_printing(&t, &exit_code)
    //table_trim(&t, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    print_tab(&t, &cl.seps, cl.ptr);

    clear_data(&t, &cl);

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
