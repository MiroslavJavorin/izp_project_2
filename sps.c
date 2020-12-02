/**
 * Project 2 - simple spreadsheet editor 2
 * Subject IZP 2020/21
 * @Author Skuratovich Aliaksandr xskura01@fit.vutbr.cz
*/

//region includes
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
//endregion


//region debug

int error_line_global = 0;

/** TURN ALL DEBUGS ON **/
#ifdef SHOWALL
    #define SHOWCORRECT /* for correction */
    #define SHOWTAB  /* for getting the table */
    #define SHOWFREE /* for free memory(functnon clear_data())*/
    #define EXPAND /* for expanding */
    #define CMDS  /* for functinos(commands) */
    #define SELECT  /* for selection commands */
    #define TRIM  /* for trimming */
#endif

/** TURN OFF DEBUGS **/
#ifdef NOTAB
#undef SHOWTAB
#endif
#ifdef NOFREE
#undef SHOWFREE
#endif
#ifdef NOTRIM
#undef TRIM
#endif
#ifdef NOEXPAND
#undef EXPAND
#endif

//endregion

/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */

#define CHECK_EXIT_CODE_IN_RUN_PROGRAM if(exit_code > 0)\
{\
    print_error_message(&exit_code);\
    clear_data(&t, &cl);\
    return exit_code;\
}

/* checks exit code in a void function */
#define CHECK_EXIT_CODE if(*exit_code > 0){ printf("(%d) err\n",__LINE__);error_line_global = __LINE__;return; }

/* check for an alloc error */
#define CHECK_ALLOC_ERR(arr) if(arr == NULL)\
{\
    *exit_code = W_ALLOCATING_ERR;\
    CHECK_EXIT_CODE\
}

#define MEMBLOCK     10 /* allocation step */
#define PTRNLEN      1001
#define NUM_TMP_SELS 10 /* max number of temo selections */
#define STR_W_FLOAT 42 /* represents size of the string with max float value */

#define NEG(n) ( n = !n ); /* negation of bool value */
#define FREE(arr) if(arr != NULL) { free(arr); } /* check if a is not NULL */
//endregion

//region enums
typedef enum
{
    NCOLS,
    WCOLS
} rtrim_opt; /* row trim option for trimming function */

/* enum that is argument for a function set_sel() to avoid error messages if given selection looks like [R,C] */
typedef enum
{
    RCRC, /* there will */
    RC,
    RCCELL,
    NOSELOPT
}sel_opt;

enum erorrs
{
    W_ALLOCATING_ERR = 1, /* error caused by a 'memory' function */
    W_SEPARATORS_ERR,     /* wrong separators have been entered  */
    VAL_UNSUPARG_ERR,     /* unsupported number of arguments     */
    VAL_UNSUPCMD_ERR,     /* wrong arguments in the cmdline  */
    ARG_UNRECARG_ERR,     /* unrecognized argument               */
    NO_SUCH_FILE_ERR,     /* no file with the entered name exists */
    LEN_UNSUPCMD_ERR,     /* unsupported len of the cmd   */
    Q_DONT_MATCH_ERR,      /* quotes dont match                   */
    UNDEF_CMD_ERR,
    UNDEF_TMPCMD_ERR,  /* TODO */
    BAD_INPUTFILE_ERR
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
    NOTH,
    CHANGESEL,
    DEF, USE, INC,
    FIND,
    MIN, MAX, SETTMP,

    SET, CLEAR, IROW, AROW, DROW, ICOL, ACOL, DCOL,
    SWAP, AVG, SUM, COUNT,LEN,
    //  TODO дополнить
} opts_t;

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
    int len;       /* size of an array */
    bool isempty;
    bool isnum;
} carr_t;

typedef struct
{
    /* upper row, left column, lower row, right column */
    int row_1, col_1, row_2, col_2;
    cmd_opt_t cmd_opt; /* defines processing option or selection option */

    opts_t proc_opt;
    char pttrn[PTRNLEN];   /* if there's a pattern in the cmd */

    bool isinit;
    bool iscolsel;  /* shows if the current command looks like [R,C] and meand cell selection */
} cmd_t;

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
    int bufsel;
    int cmds_c;       /* number of entered cmds. Represents a position of a current cmd */

    int temps_c; /* numer of temp selections */
    cmd_t tmpsel[NUM_TMP_SELS]; /* array with ppositions of temp selections */
} cl_t;

/* contains information about a row */
typedef struct
{
    carr_t *cols_v; /* columns in the row */
    int cols_c;     /* number of filled cols */
    int len;     /* size of allocated memory*/
} row_t;

/* contains information about a table */
typedef struct
{
    row_t *rows_v; /* table contains rows which contain cells */
    int row_c;    /* number of rows of the table */
    int len;   /* size of allocated memory*/
    int col_c;    /* represents number of columns after trimming */
    bool isempty;
    bool deleted;
} tab_t;
//endregion


void print_tab(tab_t *t, carr_t *seps, FILE *ptr)
{
    (void)ptr;
    if(t->deleted || t->isempty)
        return;

#ifdef SHOWTAB
    printf("\nPRINT_TAB\n%d-----------------------\n", __LINE__);
#endif
    for(int row = 0; row < t->len; row++)
    {
        for(int col = 0; col < t->rows_v[row].len; col++)
        {
            if(col)
                fputc(seps->elems_v[0], stdout);

            for(int i = 0; i < t->rows_v[row].cols_v[col].elems_c; i++)
                fputc(t->rows_v[row].cols_v[col].elems_v[i], stdout);

        }
        putc('\n', stdout);
    }
#ifdef SHOWTAB
    printf("%d-----------------------\n", __LINE__);
#endif
}

//region FREE MEM

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
    else if(c == LEN)
    {return "-len-";}
    else if(c == AVG)
    {return "-avg-";}
    else if(c == SUM)
    {return "-sum-";}
    else if(c == COUNT)
    {return "-count-";}
    else if(c == SWAP)
    {return "-swap-";}
    else if(c == SETTMP)
    {return "-set tmp-";}
    else if(c == DEF)
    {return "-def-";}
    else if(c == USE)
    {return "-use-";}
    else if(c == INC)
    {return "-inc-";}
    else return "-NaN-";
}

char *print_cmd_opt(int c)
{
    switch(c)
    {
        case NOPT:
        {
            return "-NOPT-";
        }
        case PRC:
        {
            return "-PRC-";
        }
        case SEL:
        {
            return "-SEL-";
        }
        default:
        {
            return "-NaN-";
        }
    }
}

#endif

/* frees all memory allocated by a table_t structure s */
void free_table(tab_t *t)
{

#ifdef SHOWFREE
    printf("FREE TABLE\n     %d t->len %d  t->row_c %d t->cols_c %d\n", __LINE__, t->len, t->row_c, t->col_c);
#endif
    for(int row = t->len - 1; row >= 0; row--)
    {
#ifdef SHOWFREE
        printf("(%d) row %d of len %d with %d col_c: \n", __LINE__, row, t->rows_v[row].len, t->rows_v[row].cols_c);
#endif
        for(int col = t->rows_v[row].len - 1; col >= 0; col--)
        {
#ifdef SHOWFREE
            printf(" %d", col);
#endif
            FREE(t->rows_v[row].cols_v[col].elems_v)
        }
        FREE(t->rows_v[row].cols_v)
#ifdef SHOWFREE
        printf(" freed\n");
#endif
    }
    FREE(t->rows_v)
#ifdef SHOWFREE
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

//region ARRAYS WITH CHARS FUNCS


void iscellnum(carr_t *cell)
{
    char *junk;
    cell->isnum = true;
    strtod(cell->elems_v, &junk);
    if(junk[0])
    {    cell->isnum = false;}
}

/**
 *  Adds a character to an array.
 *  If there is no more memory in the array allocate new memory
 *
 * @param arr        dynamically allocated array.
 * @param item       an item to add to an array
 * @param exit_code  can be changed if array reallocated unsuccesfully
 */
void a_carr(carr_t *arr, const char item, int *exit_code)
{
    /* if there is no more space for the new element add new space */
    if(arr->elems_c + 1 >= arr->len)
    {
        arr->len += MEMBLOCK;
        arr->elems_v = (char *)realloc(arr->elems_v, arr->len * sizeof(char));
        CHECK_ALLOC_ERR(arr->elems_v)
    }
    arr->isempty = false;
    arr->elems_v[arr->elems_c++] = item;
    arr->elems_v[arr->elems_c] = 0; /* add terminating 0 */
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

//endregion

//region CONSTRUCTORS
/* */
void carr_clear(carr_t *carr)
{
    /* there is no need to clear an empty array */
    carr->isempty = true;
    carr->isnum = false;
    memset(carr->elems_v, 0, carr->len-1);
    carr->elems_c = 0;
}

// TODO deleteme
void row_clear(row_t *row)
{
#ifdef CMDS
    printf("\n(%d) row_clear cols_c(%d) len(%d)\n", __LINE__, row->cols_c, row->len);
#endif
    for(int i = 0; i < row->len; i++) // FIXME memory
    {
        carr_clear(&row->cols_v[i]);
    }
}

void carr_ctor(carr_t *arr, int cell_len, int *exit_code)
{
    arr->elems_v = (char *)calloc(cell_len, sizeof(char));
    CHECK_ALLOC_ERR(arr->elems_v)

    arr->len = cell_len;
    arr->elems_c = 0;
    arr->isempty = true;
    arr->isnum = false;
}

/* allocating a row of size siz */
void row_ctor(row_t *r, int cols, int cell_len, int *exit_code)
{
    r->cols_v = (carr_t *)malloc(cols * sizeof(carr_t));
    CHECK_ALLOC_ERR(r->cols_v)

    /* allocate cols */
    for(int c = 0; c < cols; c++)
    {
        carr_ctor(&r->cols_v[c], cell_len, exit_code);
        CHECK_EXIT_CODE
    }
    r->len = cols;
    r->cols_c = 0;
}

/* creates a table by creating rows and columns */
void table_ctor(tab_t *t, int rows, int cols, int cel_len, int *exit_code)
{
    /* allocate thw new table */
    t->isempty = false;
    t->deleted = false;
    t->rows_v = (row_t *)malloc(rows * sizeof(row_t));
    CHECK_ALLOC_ERR(t->rows_v)

    /* allocate rows */
    for(int row = 0; row < rows; row++)
    {
        row_ctor(&t->rows_v[row], cols, cel_len, exit_code);
        CHECK_EXIT_CODE
    }
    t->row_c = 0;
    t->len = rows;
    t->col_c = cols - 1;
}
//endregion

//region TABLE RESIZE

/**
 *  Expand the table
 *
 * @param r2 Lower row
 * @param c2 Right column
 * @param exit_code if memore was not allocated
 */
void expand_tab(tab_t *t, int r2, int c2, int *exit_code) // FIXME
{
    t->deleted = false;

    /* expand_rows */
    if(r2 - 1 > t->row_c)
    {
        /* allocate new memory for rows */
        t->rows_v = (row_t *)realloc(t->rows_v, r2 * sizeof(row_t));
        CHECK_ALLOC_ERR(t->rows_v)

#ifdef EXPAND
        printf("\nEXPAND_TAB\n     %d table expanded from %d to %d rows\n", __LINE__, t->len, r2);
#endif
        t->len = r2;

        /* allocate new memory for columns */
        for(int r = t->row_c + 1; r < t->len; r++) // fixme change to  t->row_c + 1
        {
            row_ctor(&t->rows_v[r], c2, MEMBLOCK, exit_code);
            CHECK_EXIT_CODE
            t->rows_v[r].cols_c = c2 - 1;
        }
        t->row_c = t->len - 1;
    }

    /* expand collumns for all rows */
    for(int r = 0; r < r2; r++)
    {
        if(t->rows_v[r].cols_c < c2 - 1)
        {
            t->rows_v[r].len = c2;
            t->rows_v[r].cols_v = (carr_t *)realloc(t->rows_v[r].cols_v, t->rows_v[r].len * sizeof(carr_t));
            CHECK_ALLOC_ERR(t->rows_v[r].cols_v)

            for(int c = t->rows_v[r].cols_c + 1; c < t->rows_v[r].len; c++)
            {
                carr_ctor(&t->rows_v[r].cols_v[c], MEMBLOCK, exit_code);
                CHECK_EXIT_CODE
            }
            t->rows_v[r].cols_c = t->rows_v[r].len - 1;
        }
        t->col_c = t->rows_v[r].len - 1; /* now all rows have len of max col len */
    }
}

/* trims an array of characters */
void cell_trim(carr_t *arr, int *exit_code)
{
    if(arr->len >= arr->elems_c + 1)
    {
#ifdef NO
        printf("(%d)\t\t\tlen %d elems %d\n", __LINE__, arr->len, arr->elems_c); // FIXME what the fuck
#endif
        /* reallocate for elements and terminating 0 */
        arr->elems_v = (char *)realloc(arr->elems_v, (arr->elems_c + 1) * sizeof(char));
        CHECK_ALLOC_ERR(arr->elems_v)

        arr->len = (arr->elems_c) ? arr->elems_c + 1 : 1; // FIXME valgriind throw out + 1
    }
}

void row_trim(row_t *row, int siz, int *exit_code, rtrim_opt opt)
{
    /* trim all cells in the row */
#ifdef SHOWTAB
    printf("\nROW_TRIM\n     %d cols(%d)  len(%d)  \n", __LINE__, row->cols_c, row->len);
#endif

    if(row->len > siz)
    {
        for(int i = row->len - 1; i >= siz; i--)
        {
            FREE(row->cols_v[i].elems_v)
        }
        row->cols_v = (carr_t *)realloc(row->cols_v, siz * sizeof(carr_t));
        CHECK_ALLOC_ERR(row->cols_v)
        row->len = siz;
        row->cols_c = siz - 1;
    }
    else if(row->len < siz)
    {
#ifdef SHOWTAB
        printf("\nROW_TRIM\n     %d row->len %d < siz %d\n", __LINE__, row->len, siz);
#endif
        row->cols_v = (carr_t *)realloc(row->cols_v, siz * sizeof(carr_t));
        CHECK_ALLOC_ERR(row->cols_v)

        for(int c = row->cols_c + 1; c < siz; c++)
        {
            carr_ctor(&row->cols_v[c], MEMBLOCK, exit_code);
            CHECK_EXIT_CODE
        }
        row->len = siz;
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
    printf("%d len %d cols %d \n", __LINE__, row->len, row->cols_c);
#endif
}

/* find number of empty rows */
int fnum_unemp_cols(row_t *r)
{
    int empties = 0;
    for(int c = r->len - 1; c >= 0; c--)
    {
        if(r->cols_v[c].isempty)
        {empties++;}
        else
        {return r->len - empties;}
    }
    return 1;
}

/* find the longest unempty row in the table */
int find_max_unemp(tab_t *t)
{
    int max_unemp = 1; /* prevent memory error if the whole table is empty */
    int tmp = 0;

    /* initialize the row with max number of none empty columns */
    for(int r = 0; r < t->len; r++)
    {
        if(!r)
        {
            max_unemp = fnum_unemp_cols(&t->rows_v[r]);
        } else
        {
            tmp = fnum_unemp_cols(&t->rows_v[r]);
            if(tmp > max_unemp)
            {
                max_unemp = tmp;
            }
        }
    }
    if(max_unemp == 1) /* means all columns in the all rows are empty */
    {
        t->isempty = true;
    }
    return max_unemp;
}

/**
 * There is one of the separators in the cell, add the cell in quotermarks
 * @param cell
 * @param exit_code can be changed if there will be a problem with memory
 */
void quote_cell( carr_t *seps, carr_t *cell, int *exit_code)
{
    /* if there is a separator in the cell */
    for(int j = 0; j <= seps->elems_c; j++)
    {
        if(strchr(cell->elems_v, seps->elems_v[j]) != NULL && seps->elems_v[j])
        {
            cell->len += 2; /* 2 for quotermarks at the beginning and at the end */
            cell->elems_v = realloc(cell->elems_v, cell->len);
            CHECK_ALLOC_ERR(cell->elems_v)

            for(int i = cell->elems_c; i >= 0; i--)
            {
                cell->elems_v[i + 1] = cell->elems_v[i];
            }

            cell->elems_v[cell->elems_c + 1] = (cell->elems_v[0] = '\"');
            cell->elems_c += 2;
            return;
        }
    }
}

/**
 * Go through all the cells and if
 * @param t
 * @param exit_code
 */
void tab_add_quotermakrs(carr_t *seps, tab_t *t, int *exit_code)
{
#ifdef SHOWCORRECT
    printf("\nTAB_ADD_QUOTERMARKS\n     %d t->col_c %d \n\t\t cols: ", __LINE__, t->col_c);
#endif
    /* went through all rows */
    for(int r = 0; r < t->len; r++)
    {
#ifdef SHOWCORRECT
        printf("%d ", t->rows_v[r].len);
#endif
        /* process every cell in the row */
        for(int c = 0; c < t->rows_v[r].len; c++)
        {
            quote_cell(seps, &t->rows_v[r].cols_v[c], exit_code);
            CHECK_EXIT_CODE
        }
    }
}

/* delete the last empty column in the table */
void tab_trim_bef_printing(tab_t *t, int *exit_code)
{
    int max_unempt = find_max_unemp(t);

    for(int r = 0; r < t->len; r++)
    {
        row_trim(&t->rows_v[r], max_unempt, exit_code, NCOLS);
    }
    
#ifdef SHOWCORRECT
    printf("\nTAB_TRIM_BEF_PRINTING\n     %d t->col_c %d  max_unempt %d\n",__LINE__,
           t->col_c, max_unempt);
#endif
}

/* trims a table by reallocating rows and cols */
void table_trim(tab_t *t, int *exit_code)
{
    int row = 0;
    int maxlen = 0;
#ifdef SHOWTAB
    printf("\nTABLE_TRIM\n     %d len %d rows %d\n", __LINE__, t->len, t->row_c);
#endif

    if(t->row_c + 1 < t->len)
    {
        /* free unused rows that have been alloacated */
        for(row = t->len - 1; row > t->row_c; row--)
        {
            for(int col = t->rows_v[row].len - 1; col >= 0; col--)
                FREE(t->rows_v[row].cols_v[col].elems_v)
            FREE(t->rows_v[row].cols_v)
        }

        t->len = t->row_c + 1;
        /* if there were no table */
#ifdef SHOWTAB
        printf("%d t->len %d  t->row_c %d\n",__LINE__, t->len, t->row_c);
#endif
        t->rows_v = (row_t *)realloc(t->rows_v, (t->len) * sizeof(row_t));
        CHECK_ALLOC_ERR(t->rows_v)
    }

    /* trim each row */
    for(row = 0; row < t->len; row++)
    {
#ifdef SHOWTAB
        printf("(%d) untrim row %d has len (%d) and cols (%d)\n", __LINE__, row, t->rows_v[row].len,
               t->rows_v[row].cols_c);
#endif

        row_trim(&t->rows_v[row], t->rows_v[row].cols_c + 1, exit_code, WCOLS); //FIXME
        CHECK_EXIT_CODE

        if(!row || maxlen < t->rows_v[row].cols_c) // find max number of  s
        {
            maxlen = t->rows_v[row].cols_c + 1;
        }
#ifdef SHOWTAB
        printf("(%d)     trim row %d has len (%d) and cols (%d)\n", __LINE__, row, t->rows_v[row].len,
               t->rows_v[row].cols_c);
#endif
    }

    for(row = 0; row < t->len; row++)
    {
        row_trim(&t->rows_v[row], maxlen, exit_code, NCOLS); // FIXME
        CHECK_EXIT_CODE
    }

    t->col_c = maxlen - 1; //FIXME (1) -1

#ifdef SHOWTAB
    printf("%d t->cols_c %d maxlen %d", __LINE__, t->col_c, maxlen);
#endif
}
//endregion

//region GETTERS

/* check if selection meets the conditions and set it as a current selction */
void set_sel(cl_t *cl, int row1, int col1, int row2, int col2, int *exit_code, sel_opt opt)
{
    cl->cmds[cl->cellsel].iscolsel = true; /* for temp variables */

    if(cl->cmds_c != cl->cellsel && (opt != RCCELL))
    {
        /* check if the given selectinon meets the conditions */
        if(row2 < row1 || col2 < col1 ||
           row1 < cl->cmds[cl->cellsel].row_1 ||
           col1 < cl->cmds[cl->cellsel].col_1 ||
           row2 < cl->cmds[cl->cellsel].row_2 ||
           col2 < cl->cmds[cl->cellsel].col_2
                )
        {
            *exit_code = VAL_UNSUPARG_ERR;
            CHECK_EXIT_CODE
        }
    }
    if(opt == RCCELL)
    {
        cl->cellsel = cl->cmds_c;
        cl->cmds[cl->cellsel].iscolsel = true;
    }
    else if(opt == NOSELOPT)
    {
        *exit_code = VAL_UNSUPARG_ERR;
        CHECK_EXIT_CODE
    }

    /* change old current selection to the new current selection */
    cl->currsel = cl->cmds_c;
#ifdef SELECT
    printf("\nSET_SEL\n     %d   [%d,%d,%d,%d]\n", __LINE__, row1, col1, row2, col2);
#endif
    /* set a selection */
    cl->cmds[cl->currsel].row_1 = row1;
    cl->cmds[cl->currsel].col_1 = col1;
    cl->cmds[cl->currsel].row_2 = row2;
    cl->cmds[cl->currsel].col_2 = col2;

}

/* returns true is newline or eof reached */
void get_nums(carr_t *cmd, cl_t *cl, tab_t *t, int *exit_code)
{
    char *token = NULL;
    char *ptr = NULL;
    int nums[5] = {0}; /* to return an error code if smething like [a,a,a,a,a,a,a] will be entered */
    int i = 0;
    char uscore = 0;
    char dash = 0;
    cl->cmds[cl->cmds_c].iscolsel = false;
    sel_opt opt = NOSELOPT;

    /* get the first token */
    token = strtok(cmd->elems_v, ",");

    /* walk through other tokens */
    for(; i < 5 && token != NULL; i++)
    {
        /* add a number to an array */
        nums[i] = (int)(strtol(token, &ptr, 10));
        if(i < 2)
        {
            nums[i + 2] = nums[i];
        }
        if(nums[i] <= 0)
        {
            if(!strcmp(ptr, "-"))
            {
                /* according to the project specification, in this case the program should give an error */
                if(++dash == 3)
                {
                    *exit_code = VAL_UNSUPARG_ERR;
                }

                    /* in case of row selection */
                else
                {
                    nums[i] = t->len; /* processing the last row/column*/
                }
            }

                /* means only the last row or column in the table */
            else if(!strcmp(ptr, "_"))
            {
                uscore++;
                if(i == 0)
                {
                    nums[i] = 1;
                    nums[i + 2] = t->len;
                } else if(i == 1)
                {
                    nums[i] = 1;
                    nums[i + 2] = t->col_c + 1;
                }
            }

                /* if there is another string literal */
            else
            {
                *exit_code = W_SEPARATORS_ERR;
            }

            CHECK_EXIT_CODE
        }
        else if(ptr[0] != 0)
        {
            *exit_code = VAL_UNSUPCMD_ERR;
        }
        token = strtok(NULL, ",");
    }

    CHECK_EXIT_CODE
    /* if entered selection looks like [R1,C1] */
    if(i == 2)
    {
        if(dash)
        {
            *exit_code = VAL_UNSUPARG_ERR;
        }
            /* set a new cell and current selection */
        else if(!uscore)
        {
            opt = RCCELL;
        }
        else opt = RC;
    }

        /* if entered selection looks like [R1,C1,R2,C2] */
    else if(i == 4)
    {
        if(uscore)
        {
            *exit_code = VAL_UNSUPCMD_ERR;
        }
        /* finally set the selection */
        opt = RCRC;
    }
    else
    {
        *exit_code = ARG_UNRECARG_ERR;
    }

    set_sel(cl, nums[0], nums[1], nums[2], nums[3], exit_code, opt);
#ifdef SELECT
    printf("(%d) extracted selection: [%d,%d,%d,%d]\n", __LINE__, nums[0], nums[1], nums[2], nums[3]);
#endif
    CHECK_EXIT_CODE
}


bool get_cell(carr_t *col, cl_t *cl, int *exit_code)
{
    char buff_c;
    bool quoted = false;
    bool backslashed = false;

    for(col->elems_c = 0; ; )
    {
        buff_c = (char)fgetc(cl->ptr);

        if(buff_c == '\\')
        {
            if(backslashed)
            {
                a_carr(col, buff_c, exit_code);
                a_carr(col, buff_c, exit_code);
            }
            //a_carr(col, buff_c, exit_code);
            NEG(backslashed)
        }

        else if(buff_c == '\"')
        {
            if(backslashed)
            {
                a_carr(col, '\\', exit_code);
                a_carr(col, buff_c, exit_code);
            }
            NEG(quoted)
        }

        else if(buff_c == '\n' || feof(cl->ptr))
        {
            if(backslashed && quoted){*exit_code = BAD_INPUTFILE_ERR;}
#ifdef SHOWTAB
            printf("(%d)  \t\t\t%s\n", __LINE__, col->elems_v);
#endif
            return true; // FIXME ?
        }

        /* if the char is a separator and it is not quoted */
        else if(strchr(cl->seps.elems_v, buff_c) != NULL && !quoted && !backslashed)
        {
#ifdef SHOWTAB
            printf("(%d)  \t\t\t%s\n", __LINE__, col->elems_v);
#endif
            return false;
        }

        else
        {
            if(backslashed){NEG(backslashed)}
            a_carr(col, buff_c, exit_code);
        }
    }
}

/* gets a row from the file and adds it to the table */
void get_row(row_t *row, cl_t *cl, int *exit_code)
{
    int end = 0;
    for(row->cols_c = 0; !feof(cl->ptr); row->cols_c++)
    {
        if(row->cols_c == row->len)
        {
            row->len += MEMBLOCK;
            row->cols_v = (carr_t *)realloc(row->cols_v, row->len * sizeof(carr_t));
            CHECK_ALLOC_ERR(row->cols_v)

            for(int c = row->len - 1; c >= row->cols_c; c--)
            {
                /* create a col */
                carr_ctor(&row->cols_v[c], MEMBLOCK, exit_code);
                CHECK_EXIT_CODE
            }
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
            printf("(%d)  \tlen(%d) cols(%d)\n", __LINE__, row->len, row->cols_c);
#endif
            return;
        }
    }
}

void get_table(const int *argc, const char **argv, tab_t *t, cl_t *cl, int *exit_code)
{
    if((cl->ptr = fopen(argv[*argc - 1], "r+")) == NULL)
    {
        *exit_code = NO_SUCH_FILE_ERR;
        return;
    }

        /* if file has been opened successfully */
    else
    {
        /* allocate a new empty table */
        table_ctor(t, 1, 1, 1, exit_code);
        CHECK_EXIT_CODE
#ifdef SHOWTAB
        printf("\nGET_TABLE\n     %d t->row_c %d  t->len %d  t->col_c %d\n", __LINE__, t->row_c, t->len, t->col_c);
#endif
        /* write the table to the structure */
        for(t->row_c = 0; !feof(cl->ptr); t->row_c++)
        {
            /* realloc the table to the new size */
            if(t->row_c == t->len)
            {
                t->len += MEMBLOCK;
                t->rows_v = (row_t *)realloc(t->rows_v, (t->len) * sizeof(row_t));
                CHECK_ALLOC_ERR(t->rows_v)

                for(int rc = t->len - 1; rc >= t->row_c; rc--) // fixed DONT TOUCH!!
                {
                    /* create a row */
                    row_ctor(&t->rows_v[rc], MEMBLOCK, MEMBLOCK, exit_code);
                    CHECK_EXIT_CODE
                }
            }
            get_row(&t->rows_v[t->row_c], cl, exit_code);
            CHECK_EXIT_CODE
        }
        t->row_c -= 2; /* decrase number of rows */
#ifdef SHOWTAB
        printf("\n     %d  len(%d) row_c(%d)\n", __LINE__, t->len, t->row_c);
#endif
        if(t->row_c >= 0)
        {
            /* align a table */
            table_trim(t, exit_code);
            CHECK_EXIT_CODE
        }
        else
        {
            t->row_c = 0;
        }
    }
}
//endregion

//region functions


/**
 * Set the cell value to the string STR.
 * The string STR can be enclosed in quotation marks and can contain special characters preceded by a slash
 */
void set_f(tab_t *t, int r, int c, char *pattern, int *exit_code) // TODO make a separate argument from a
// pattern
{
    /* expand_tab if it is neccessary */
    expand_tab(t, r, c, exit_code);
    CHECK_EXIT_CODE

#ifdef CMDS
    printf("\nSET_F\n");
#endif
    int len = 0;
    carr_clear(&t->rows_v[r - 1].cols_v[c - 1]); /* clear a cell */
    len = (int)strlen(pattern);

    for(int p = 0; p < len; p++)
    {
        a_carr(&t->rows_v[r - 1].cols_v[c - 1], pattern[p], exit_code);
        CHECK_EXIT_CODE
    }

#ifdef CMDS
    printf("%d len %d for pattern -->%s<--\n", __LINE__, len, pattern);
#endif
}

/**
 * sets the current cell selection to the temporary variable _
 * (only which cells are selected, not their contents)
 */
void set_tmp_f(cl_t *cl, int *exit_code )
{
    cl->bufsel = cl->currsel; /* save the current selection to the buffer selection */
}

/**
 * define a tepm selection
 */
void def_f(cl_t *cl)
{
#define pos cl->cmds[cl->cmds_c].row_1

    /* copy a current cell selection to the command */
    cl->tmpsel[pos].row_2 = (cl->tmpsel[pos].row_1 = cl->tmpsel[cl->cellsel].row_1);
    cl->tmpsel[pos].col_2 = (cl->tmpsel[pos].col_1 = cl->tmpsel[cl->cellsel].col_1);
    cl->temps_c++; /* finally increase number of temp selections */
#ifdef CMDS
    printf("\nDEF_F\n     %d  temp sel defined [%d,%d]\n     %d n  num of temp sels %\n",__LINE__,
            cl->tmpsel[pos].row_1,
           cl->tmpsel[pos].col_1, __LINE__, cl->temps_c);
#endif

#undef pos
}

/**
 *  The current cell will be set to the value from
 *  the temp variable X. If temp variable contains more than 1 cell, the cureent cell will be set to the 1. cell
 *  from the tmp variable X
 *  (where X identifies the temporary variable _0 to _9)
 * @param cl
 * @param exit_code can be changed 1) if the commad with given number have not been defined
 */
void use_f(cl_t *cl, tab_t *t, int *exit_code)
{
#define pos cl->cmds[cl->cmds_c].row_1 /* shows position in array of temp selections */
#define row cl->tmpsel[pos].row_1
#define col cl->tmpsel[pos].col_1

    /* check if the command is initialized */
    if(cl->tmpsel[pos].isinit)
    {
        set_f(t, cl->cmds[cl->cellsel].row_1, cl->cmds[cl->cellsel].col_1 , t->rows_v[row].cols_v[col].elems_v,exit_code );
    }else *exit_code = UNDEF_TMPCMD_ERR;
    CHECK_EXIT_CODE
#undef col
#undef row
#undef pos
}

/**
 *   inc _X - the numeric value in the temporary variable will be incremented by 1.
 *   If the temporary variable does not contain a number, the resulting value of the variable will be set to 1.
 * @param cd
 * @param t
 * @param exit_code
 */
void inc_f(cl_t *cl, tab_t *t, int *exit_code)
{
#define pos cl->cmds[cl->cmds_c].row_1 /* shows position in array of temo selections */
#define row cl->tmpsel[pos].row_1
#define col cl->tmpsel[pos].col_1

    /* check if the command is initialized */
    if(cl->tmpsel[pos].isinit)
    {
        expand_tab(t, row, col, exit_code);
        CHECK_EXIT_CODE
        char tmpresult[STR_W_FLOAT] = {0}; /* create a string where result will be written */

        /* if cell contains a number increase it by 1*/
        if(t->rows_v[row].cols_v[col].isnum)
        {
            /* extract number from the cell*/
            float num_in_cell = (float)strtod(t->rows_v[row].cols_v[col].elems_v, NULL);
            num_in_cell++; /* increase the number by 1 */

            sprintf(tmpresult, "%g", num_in_cell); /* add a result to the temp string with result */
        }
        /* means temporary variable does not contain a number set 1 in the cell */
        else
        {
            tmpresult[0] = '1';
        }

        set_f(t, row, col, tmpresult, exit_code);
        t->rows_v[row].cols_v[col].isnum = true;

    } else *exit_code = UNDEF_TMPCMD_ERR;
    CHECK_EXIT_CODE
#undef col
#undef row
#undef pos
}

/**
 * len [R, C] - stores the string length of the currently selected cell in the cell on row R and column C.
 */
void len_f(cl_t *cl, tab_t *t, int *exit_code)
{
#define r cl->cmds[cl->cellsel].row_1
#define c cl->cmds[cl->cellsel].col_1
    sprintf(cl->cmds[cl->cmds_c].pttrn, "%d",(int)strlen(t->rows_v[r].cols_v[c].elems_v));
    set_f(t, r, c, cl->cmds[cl->cmds_c].pttrn ,exit_code);
#undef r
#undef c
}

void count_f(cl_t *cl, tab_t *t, int *exit_code)
{
    int nempties = 0;

    for(int r = cl->cmds[cl->currsel].row_1 - 1; r < cl->cmds[cl->currsel].row_2; r++)
    {
        for(int c = cl->cmds[cl->currsel].col_1 - 1; c < cl->cmds[cl->currsel].col_2; c++)
        {
            if(!t->rows_v[r].cols_v[c].isempty)
            {
                nempties++;
            }
        }
    }
    sprintf(cl->cmds[cl->cmds_c].pttrn, "%d", nempties);
    set_f(t, cl->cmds[cl->cmds_c].row_1, cl->cmds[cl->cmds_c].col_1, cl->cmds[cl->cmds_c].pttrn, exit_code);
#ifdef CMDS
    printf("\nCOUNT_F\n     %d  count %d\n",__LINE__, nempties);
#endif
}


/**
 * sum [R, C] - stores the sum of values of selected cells
 * (corresponding to the format %g in printf) in the cell on row R and column C.
 * Selected cells without a number will be ignored (as if they were not selected)
 *
 * * avg [R, C] - same as sum, but the arithmetic mean of the selected cells is stored
 */
void avg_sum_f(cl_t *cl, tab_t *t, int *exit_code)
{
    int numcels = 0;
    float result = 0;
#ifdef CMDS
    printf("\nAVG_SUM_F\n     %d for cursel [%d,%d,%d,%d]\n",__LINE__, cl->cmds[cl->currsel].row_1,
            cl->cmds[cl->currsel]
    .row_2,
           cl->cmds[cl->currsel].col_1, cl->cmds[cl->currsel].col_2 );
#endif

    for(int r = cl->cmds[cl->currsel].row_1 - 1; r < cl->cmds[cl->currsel].row_2; r++)
    {
        for(int c = cl->cmds[cl->currsel].col_1 - 1; c < cl->cmds[cl->currsel].col_2; c++)
        {
            if(t->rows_v[r].cols_v[c].isnum)
            {
                numcels++;
                result += (float)strtod(t->rows_v[r].cols_v[c].elems_v, NULL);
            }
        }
    }

    /* if number of cells with number is not a 0, */
    if(numcels)
    {
        if(cl->cmds[cl->cmds_c].proc_opt == AVG)
        {
            result /= (float)numcels;
        }
        sprintf(cl->cmds[cl->cmds_c].pttrn, "%g", result); /* write the result to pattern */
    }
    else
    {
        cl->cmds[cl->cmds_c].pttrn[0] = '0';
    }
    set_f( t, cl->cmds[cl->cmds_c].row_1, cl->cmds[cl->cmds_c].col_1, cl->cmds[cl->cmds_c].pttrn, exit_code);
}


/**
 * swap [R,C] - swaps the contents of the selected cell with the cell from the last cell selection
 */
void swap_f(cl_t *cl, tab_t *t)
{
    /* create macros to use less mempry and to make the code more readable */
#define r cl->cmds[cl->cellsel].row_1 - 1
#define c cl->cmds[cl->cellsel].col_1 - 1
#define r1 cl->cmds[cl->cmds_c].row_1 - 1
#define c1 cl->cmds[cl->cmds_c].col_1 - 1

#ifdef CMDS
    printf("\nSWAP_F\n     %d bef %d %d | %d %d\n",__LINE__, t->rows_v[r].cols_v[c].len, t->rows_v[r].cols_v[c].elems_c,
           t->rows_v[r1].cols_v[c1].len, t->rows_v[r1].cols_v[c1].elems_c);
#endif

    /* */
    char *tmp = t->rows_v[r].cols_v[c].elems_v;
    int tempum = t->rows_v[r].cols_v[c].len;
    //int elems_c = t->rows_v[r].cols_v[c].elems_c;

    /* copy 2nd row to the first row */
    t->rows_v[r].cols_v[c].elems_v = t->rows_v[r1].cols_v[c1].elems_v;
    t->rows_v[r].cols_v[c].len     = t->rows_v[r1].cols_v[c1].len;
    t->rows_v[r1].cols_v[c1].len = tempum;
    tempum = t->rows_v[r].cols_v[c].elems_c;
    t->rows_v[r].cols_v[c].elems_c = t->rows_v[r1].cols_v[c1].elems_c;
    t->rows_v[r1].cols_v[c1].elems_c = tempum;

    t->rows_v[r1].cols_v[c1].elems_v = tmp;

#ifdef CMDS
    printf("%d aft %d %d | %d %d\n", __LINE__, t->rows_v[r].cols_v[c].len, t->rows_v[r].cols_v[c].elems_c,
           t->rows_v[r1].cols_v[c1].len, t->rows_v[r1].cols_v[c1].elems_c);
#endif

#undef c1
#undef r1
#undef c
#undef r
}


void min_max_f(cl_t *cl, tab_t *t, int *exit_code, int opt)
{
    /* start */
    int r = cl->cmds[cl->currsel].row_1 - 1;


    int targ_row = -1, targ_col = -1;
    double mval = 0, mtempval = 0;
    bool found = false;
#ifdef CMDS
    printf("\nMIN_MAX\n     %d  current selection [%d,%d,%d,%d] \n%d  lower row = %d  roght col = %d\n",__LINE__,
           cl->cmds[cl->currsel].row_1, cl->cmds[cl->currsel].col_1, cl->cmds[cl->currsel].row_2,
           cl->cmds[cl->currsel].col_2,
            __LINE__, cl->cmds[cl->currsel].row_2,cl->cmds[cl->currsel].col_2);
#endif

    /* walk through all selected rows */
    for(; r < cl->cmds[cl->currsel].row_2; r++)
    {
        /* walk through all selected columns */
        for(int c = cl->cmds[cl->currsel].col_1 - 1; c < cl->cmds[cl->currsel].col_2; c++)
        {
            /* if there is a number in the column */
            if(t->rows_v[r].cols_v[c].isnum)
            {
                if(!found)
                {
                    found = true;
                    mval = strtod(t->rows_v[r].cols_v[c].elems_v, NULL);
                    targ_row = r, targ_col = c;
                }

                if(opt == MAX)
                {
                    mtempval = strtod(t->rows_v[r].cols_v[c].elems_v, NULL);
                    if(mtempval > mval)
                    {
                        mval = mtempval;
                        targ_row = r;
                        targ_col = c;
                    }
                } else if(opt == MIN)
                {
                    mtempval = strtod(t->rows_v[r].cols_v[c].elems_v, NULL);
                    if(mtempval < mval)
                    {
                        mval = mtempval;
                        targ_row = r;
                        targ_col = c;
                    }
                } else return;
            }
        }
    }

    /* if targ_row and targ_col havn't been changed that means there is no cell in the selection with a number in it */
    if(targ_row != -1 && targ_col != -1)
    {
#ifdef CMDS
        printf("%d new selection [%d,%d,%d,%d]", __LINE__, targ_row + 1, targ_col + 1, targ_row + 1, targ_col + 1);
#endif
        cl->cellsel = cl->cmds_c;
        set_sel(cl,targ_row + 1, targ_col + 1, targ_row + 1, targ_col + 1, exit_code, RCCELL);
    }
}


void find_f(cl_t *cl, tab_t *t, int *exit_code)
{
#ifdef CMDS
    printf("\nFIND_F\n     %d HERE WE ARE \n", __LINE__);
#endif
    int r = cl->cmds[cl->currsel].row_1 - 1;

    /* walk through all selected rows and columns */
    for(; r < cl->cmds[cl->currsel].row_2; r++)
    {
        for(int c = cl->cmds[cl->currsel].col_1 - 1; c < cl->cmds[cl->currsel].col_2; c++)
        {
            if(strstr(t->rows_v[r].cols_v[c].elems_v, cl->cmds[cl->cmds_c].pttrn) != NULL)
            {
                cl->cellsel = cl->cmds_c;
                set_sel(cl,r + 1, c + 1, r + 1, c + 1, exit_code, RCCELL);
#ifdef CMDS
                printf("%d pattern %s found, sel havе been changed to [%d,%d,%d,%d]\n", __LINE__, cl->cmds[cl->cmds_c]
                        .pttrn, r + 1, c + 1, r + 1, c + 1);
#endif
                return;
            }
        }
    }
}


void clear_f(cl_t *cl, tab_t *t)
{
    int row_fr = cl->cmds[cl->currsel].row_1 - 1;

#ifdef CMDS
    printf("\nCLEAR\n     %d  currsel: [%d,%d,%d,%d]\n"
           "           row_fr %d  col_fr %d\n"
           "           row_to %d  col_to  %d\n",__LINE__, cl->cmds[cl->currsel].row_1, cl->cmds[cl->currsel]
    .col_1, cl->cmds[cl->currsel].row_2, cl->cmds[cl->currsel].col_2, row_fr, cl->cmds[cl->currsel].col_1 - 1,
           cl->cmds[cl->currsel].row_2, cl->cmds[cl->currsel].col_2);
#endif
    for(; row_fr < cl->cmds[cl->currsel].row_2; row_fr++)
    {
        for(int col_fr = cl->cmds[cl->currsel].col_1 - 1; col_fr < cl->cmds[cl->currsel].col_2; col_fr++)
        {
            carr_clear(&t->rows_v[row_fr].cols_v[col_fr]);
        }
    }
}

void irow_arow_f(cl_t *cl, tab_t *t, int *exit_code, int opt)
{
    int upper_b = 0;
    if(opt == IROW)
    {
        upper_b = cl->cmds[cl->currsel].row_1 - 1;
    } else if(opt == AROW)
    {
        upper_b = cl->cmds[cl->currsel].row_2;
    } else return;

    /* allocate memory for a new row */
    t->rows_v = (row_t *)realloc(t->rows_v, (t->len + 1) * sizeof(row_t));
    CHECK_ALLOC_ERR(t->rows_v)

    /* move all rows */
    for(int i = t->len - 1; i >= upper_b; i--)
    {
        t->rows_v[i + 1] = t->rows_v[i];
    }
    t->len++;
    t->row_c++;
#ifdef CMDS
    printf("\nIROW_AROW_F\n     %d  t->col_c %d\n", __LINE__,t->col_c);
#endif

    /* create a row */
    row_ctor(&t->rows_v[upper_b], (t->col_c < 2) ? 1 : t->col_c - 1, MEMBLOCK, exit_code);
    CHECK_EXIT_CODE
}


void drow_f(cl_t *cl, tab_t *t, int *exit_code) //FIXME testme
{
    int from = cl->cmds[cl->currsel].row_1;
    int to = cl->cmds[cl->currsel].row_2;
    /* if the whole table is about to be deleted  */
    if(from == 1 && to >= t->len)
    {
        t->deleted = true;
    }

    if(from > t->len)
    {
        to = t->len;

        /* it means selected rows have already been deleted */
        if(from > t->len)
        {
            return;
        }
    }

    int diff = to - from + 1;

#ifdef CMDS
    printf("\nDROW\n     %d  from = %d to = %d\n", __LINE__, from, to);
#endif

    for(int r = from - 1; r < to; r++)
    {
        for(int c = t->rows_v[r].len - 1; c >= 0; c--)
        {
            FREE(t->rows_v[r].cols_v[c].elems_v)
        }
        FREE(t->rows_v[r].cols_v)
    }

    for(int r = from - 1; r + diff < t->len; r++)
    {
        t->rows_v[r] = t->rows_v[r + diff];
    }
    t->row_c -= diff;
    t->len -= diff;

#ifdef CMDS
    printf("     %d   t->row_c %d   t->len %d\n", __LINE__, t->row_c, t->len);
#endif
    /* re-allocate memory */
    t->rows_v = (row_t *)realloc(t->rows_v, t->len * sizeof(row_t));
    CHECK_ALLOC_ERR(t->rows_v)
}


/* add or insert a column before or after the selection*/
void icol_acol_f(cl_t *cl, tab_t *t, int *exit_code, int opt)
{
    int left_b = 0;
    if(opt == ICOL)
    {
        left_b = cl->cmds[cl->currsel].col_1 - 1;
    } else if(opt == ACOL)
    {
        left_b = cl->cmds[cl->currsel].col_2;
    } else return;

    /* walk through all selected rows */
    for(int r = cl->cmds[cl->currsel].row_1 - 1; r < cl->cmds[cl->currsel].row_2; r++)
    {
        t->rows_v[r].len++;

        /* increase size of each row */
        if(r >= cl->cmds[cl->currsel].row_1 - 1 && r < cl->cmds[cl->currsel].row_2)
        {
            t->rows_v[r].cols_v = (carr_t *)realloc(t->rows_v[r].cols_v, (t->rows_v[r].len) * sizeof(carr_t));

            /* move all rows right */
            for(int c = t->rows_v[r].len - 1; c > left_b; c--)
            {
                t->rows_v[r].cols_v[c] = t->rows_v[r].cols_v[c - 1];
            }

            /* insert a column itself */
            carr_ctor(&t->rows_v[r].cols_v[left_b], MEMBLOCK, exit_code);
            CHECK_EXIT_CODE
            t->rows_v[r].cols_c++;
        } else
        {
            /* increase size of another row by increasing the number of cells and allocating a new one*/
            row_trim(&t->rows_v[r], t->rows_v[r].len, exit_code, NCOLS);
        }
        CHECK_EXIT_CODE
    }
    t->col_c++;
}


/* in an existing selection, it finds the cell with the min/max numeric value and sets the selection to it */
void dcol_f(cl_t *cl, tab_t *t, int *exit_code)
{
    /* Tn the first option, a whole row is about to be deleted */
    if(cl->cmds[cl->currsel].col_1 == 1 && cl->cmds[cl->currsel].col_2 >= t->col_c + 1)
    {
        /* delete a * whole row */
        drow_f(cl, t, exit_code);
        CHECK_EXIT_CODE
    }

        /* The second option removes only some of the columns*/
    else
    {
        int r_from = cl->cmds[cl->currsel].row_1;
        int r_to = cl->cmds[cl->currsel].row_2;

        int c_from = cl->cmds[cl->currsel].col_1;
        int c_to = cl->cmds[cl->currsel].col_2;


        if(r_to > t->len)
        {
            /* it means the lower row is already deleted */
            r_to = t->len;

            /* it means all rows from selection are already deleted */
            if(r_from > r_to)
            {
                return;
            }
        }
        int diff = 0;

        /* walk through all rows */
        for(int r = r_from - 1; r >= r_to - 1; r--)
        {
            if(cl->cmds[cl->currsel].col_2 > t->rows_v[r].len)
            {
                c_from = t->rows_v[r].len;
                if(cl->cmds[cl->currsel].col_1 > c_from) /* it means all columns have already been deleted */
                {
                    continue;
                }
            }

            printf("%d  clear from %d to %d\n", __LINE__, c_from, c_to);

            /* first free all allocated cells */
            for(int c = c_from - 1; c < c_to; c++)
            {
                FREE(t->rows_v[r].cols_v[c].elems_v)
            }
            diff = c_to - c_from + 1;

            /* shift all columns */
            for(int c = c_from - 1; c + diff < t->rows_v[r].len; c++)
            {
                t->rows_v[r].cols_v[c] = t->rows_v[r].cols_v[c + diff];
            }
            t->rows_v[r].cols_c -= diff;
            t->rows_v[r].len -= diff;
        }
    }
}
//endregion


//region PROCESSING

/**
 * Call functions for processing the selection
 */
void process_sel(cl_t *cl, tab_t *t, int *exit_code)
{
    if(cl->cmds[cl->cmds_c].proc_opt == FIND)
    {find_f(cl, t, exit_code);}

    else if(cl->cmds[cl->cmds_c].proc_opt == DEF)
    {def_f(cl);}

    else if(cl->cmds[cl->cmds_c].proc_opt == USE)
    {use_f(cl, t, exit_code);}

    else if(cl->cmds[cl->cmds_c].proc_opt == INC)
    {inc_f(cl, t, exit_code);}

        /* set column in range of the selection by numerical value */
    else if(cl->cmds[cl->cmds_c].proc_opt == MIN || cl->cmds[cl->cmds_c].proc_opt == MAX)
    {min_max_f(cl, t, exit_code, cl->cmds[cl->cmds_c].proc_opt);}

    else if(cl->cmds[cl->cmds_c].proc_opt == SETTMP)
    {set_tmp_f(cl, exit_code);}
}

/**
 *  Call functions for processing the table 
 */
void process_table(cl_t *cl, tab_t *t, int *exit_code)
{
#ifdef CMDS
    printf("(%d)opt %s for cmd_c %d\n\n ", __LINE__, print_opt(cl->cmds[cl->cmds_c].proc_opt), cl->cmds_c);
#endif
    if(cl->cmds[cl->cmds_c].proc_opt == LEN)
    {len_f(cl, t, exit_code);}

    else if(cl->cmds[cl->cmds_c].proc_opt == COUNT)
    {count_f(cl, t, exit_code);}

    else if(cl->cmds[cl->cmds_c].proc_opt >= AVG && cl->cmds[cl->cmds_c].proc_opt <= SUM)
    {avg_sum_f(cl, t, exit_code);}


    else if(cl->cmds[cl->cmds_c].proc_opt == SWAP)
    {swap_f(cl, t);}


    else if(cl->cmds[cl->cmds_c].proc_opt == SET)
    {set_f(t,cl->cmds[cl->cellsel].row_1, cl->cmds[cl->cellsel].col_1, cl->cmds[cl->cellsel].pttrn, exit_code);}


    if(cl->cmds[cl->cmds_c].proc_opt == AROW || cl->cmds[cl->cmds_c].proc_opt == IROW)
    {irow_arow_f(cl, t, exit_code, cl->cmds[cl->cmds_c].proc_opt);}

    else if(cl->cmds[cl->cmds_c].proc_opt == DROW)
    {drow_f(cl, t, exit_code);}

    else if(cl->cmds[cl->cmds_c].proc_opt == ICOL || cl->cmds[cl->cmds_c].proc_opt == ACOL)
    {icol_acol_f(cl, t, exit_code, cl->cmds[cl->cmds_c].proc_opt);}

    else if(cl->cmds[cl->cmds_c].proc_opt == CLEAR)
    {clear_f(cl, t);}

    else if(cl->cmds[cl->cmds_c].proc_opt == DCOL)
    {dcol_f(cl, t, exit_code);}

    /* actually this statement must not be reached */
    else{*exit_code = UNDEF_CMD_ERR;}

}
//endregion

//region cmdLINE ARGS PARSING

void add_ptrn(carr_t *cmd, char *arr, char *ptrn, int *exit_code)
{
    if(strlen(cmd->elems_v + strlen(ptrn)) < PTRNLEN)
    {
        /* write a pattern to the structure */
        memcpy(arr, cmd->elems_v + strlen(ptrn), strlen(cmd->elems_v + strlen(ptrn)));
#ifdef CMDS
        printf("\nINIT_WSPASED_CMD\n     %d pattern -->%s<--", __LINE__, arr);
#endif
    }
        /* pattern is too long */
    else
    {
        *exit_code = LEN_UNSUPCMD_ERR;
        CHECK_EXIT_CODE
    }
}

/* TODO documentation */
void extract_nums(cl_t *cl, char n_extr_nums[PTRNLEN], int *exit_code)
{
    char *token = NULL;
    char *ptr = NULL;
    int nums[5] = {0}; /* to return an error code if smething like [a,a,a,a,a,a,a] will be entered */
    int i = 0;
    cl->cmds[cl->cmds_c].iscolsel = false;

#ifdef CMDS
    printf("\nEXTRACT_NUMS\n     %d pattern -->%s<--\n", __LINE__, n_extr_nums);
#endif
    /* get the first token */
    token = strtok(n_extr_nums, ",");

    /* walk through other tokens */
    for( ; i < 2 && token != NULL; i++)
    {
        /* add a number to an array */
        nums[i] = (int)(strtol(token, &ptr, 10));
        if(nums[i] <= 0 || ptr[0] != 0)
        {
            *exit_code = VAL_UNSUPCMD_ERR;
            CHECK_EXIT_CODE
        }
        /* check whethe the number represents row or column and change structure */
        if(i == 0)
        {
            cl->cmds[cl->cmds_c].row_1 = (cl->cmds[cl->cmds_c].row_2 = nums[i]);
        }
        else if(i == 1)
        {
            cl->cmds[cl->cmds_c].col_1 = (cl->cmds[cl->cmds_c].col_2 = nums[i]);
        }
        else
        {
            *exit_code = VAL_UNSUPARG_ERR;
        }
        token = strtok(NULL, ",");
    }

#ifdef SELECT
    printf("\nEXTRACT_NUMS\n     %d extracted selection: [%d,%d,%d,%d]\n", __LINE__,
           cl->cmds[cl->cmds_c].row_1, cl->cmds[cl->cmds_c].col_1,cl->cmds[cl->cmds_c].row_2, cl->cmds[cl->cmds_c].col_2);
#endif
    CHECK_EXIT_CODE
}


void tmp_cmds_extract_nums(cl_t *cl, char n_extr_nums[PTRNLEN], int *exit_code)
{
    char *junk = NULL;
    int num = 0;
    num = (int)strtol(n_extr_nums, &junk, 1);

    /* if there is a wrong number in the command or number is too large */
    if(junk[0] || num > NUM_TMP_SELS - 1)
    {
        *exit_code = VAL_UNSUPARG_ERR;
        CHECK_EXIT_CODE
    }

    /* partly initialize the command */
    cl->cmds[cl->cmds_c].row_1 = num; /* represents the number of temp_sel command */
    cl->tmpsel[num].isinit = true;
#ifdef CMDS
    printf("\nMP_CMDS_EXTRACT_NUMS\n     %d  tmo com num %d initialized",__LINE__, num);
#endif
}



/**
 *
 * @param cmd
 * @param cl
 * @param t
 * @param exit_code
 */
void init_wspased_cmd(carr_t *cmd, cl_t *cl, int *exit_code)
{
    /* create an array for the part of command after whitespace */
    char n_extr_nums[PTRNLEN] = {0};

    /* add a command to the structure  */
    /* commands use a string as an argument */
    /* The string STR can be enclosed in quotation marks and can contain special characters preceded by a slash */
    if(!strncmp("find ", cmd->elems_v, strlen("find ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = FIND;
        cl->cmds[cl->cmds_c].cmd_opt = SEL;
        add_ptrn(cmd, cl->cmds[cl->cmds_c].pttrn, "set ", exit_code);
        CHECK_EXIT_CODE
    }
    else if(!strncmp("set ", cmd->elems_v, strlen("set ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = SET;
        cl->cmds[cl->cmds_c].cmd_opt = PRC;
        add_ptrn(cmd, cl->cmds[cl->cmds_c].pttrn, "set ", exit_code);
        CHECK_EXIT_CODE
    }

    /* commands use a cell as an argument */
    else if(!strncmp("swap ", cmd->elems_v, strlen("swap ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = SWAP;
        add_ptrn(cmd, n_extr_nums, "swap ", exit_code);
    }
    else if(!strncmp("sum ", cmd->elems_v, strlen("sum ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = SUM;
        add_ptrn(cmd, n_extr_nums, "sum ", exit_code);
    }
    else if(!strncmp("avg ", cmd->elems_v, strlen("avg ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = AVG;
        add_ptrn(cmd, n_extr_nums, "avg ", exit_code);
    }
    else if(!strncmp("count ", cmd->elems_v, strlen("count ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = COUNT;
        add_ptrn(cmd, n_extr_nums, "count ", exit_code);
    }
    else if(!strncmp("len ", cmd->elems_v, strlen("len ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = LEN;
        add_ptrn(cmd, n_extr_nums, "len ", exit_code);
    }

    /* functinos for working with temporary variables */
    else if(!strncmp("def ", cmd->elems_v, strlen("def ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = DEF;
        add_ptrn(cmd, n_extr_nums, "def _", exit_code);}
    else if(!strncmp("use ", cmd->elems_v, strlen("use ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = USE;
        add_ptrn(cmd, n_extr_nums, "use _", exit_code);}
    else if(!strncmp("inc ", cmd->elems_v, strlen("inc ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = INC;
        add_ptrn(cmd, n_extr_nums, "inc _", exit_code);}


    if(cl->cmds[cl->cmds_c].proc_opt >= SET && cl->cmds[cl->cmds_c].proc_opt <= LEN)
    {
        cl->cmds[cl->cmds_c].cmd_opt = PRC;

        /* extract numbers from commands working with temporary variables */
        if(cl->cmds[cl->cmds_c].proc_opt >= DEF && cl->cmds[cl->cmds_c].proc_opt <= INC)
        {
            tmp_cmds_extract_nums(cl, n_extr_nums, exit_code);
        }

        /* command set have no numbers to extract, other have */
        if(cl->cmds[cl->cmds_c].proc_opt != SET)
        {
            extract_nums(cl, n_extr_nums, exit_code);
        }
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
void init_n_wspased_cmd(carr_t *cmd, cl_t *cl, tab_t *t, int *exit_code)
{
    /* number of occurances */
    char *currcom = (char *)calloc(cmd->elems_c, sizeof(char));
    CHECK_ALLOC_ERR(currcom)

    if(!strcmp(cmd->elems_v, "irow"))
    {cl->cmds[cl->cmds_c].proc_opt = IROW;}
    else if(!strcmp(cmd->elems_v, "arow"))
    {cl->cmds[cl->cmds_c].proc_opt = AROW;}
    else if(!strcmp(cmd->elems_v, "drow"))
    {cl->cmds[cl->cmds_c].proc_opt = DROW;}
    else if(!strcmp(cmd->elems_v, "icol"))
    {cl->cmds[cl->cmds_c].proc_opt = ICOL;}
    else if(!strcmp(cmd->elems_v, "acol"))
    {cl->cmds[cl->cmds_c].proc_opt = ACOL;}
    else if(!strcmp(cmd->elems_v, "dcol"))
    {cl->cmds[cl->cmds_c].proc_opt = DCOL;}
    else if(!strcmp(cmd->elems_v, "clear"))
    {cl->cmds[cl->cmds_c].proc_opt = CLEAR;}
    else if(!strcmp(cmd->elems_v, "min"))
    {cl->cmds[cl->cmds_c].proc_opt = MIN;}
    else if(!strcmp(cmd->elems_v, "max"))
    {cl->cmds[cl->cmds_c].proc_opt = MAX;}
    else if(!strcmp(cmd->elems_v, "set"))
    {cl->cmds[cl->cmds_c].proc_opt = SETTMP;}
        /* revert back the selection that was before the temporary selection was applied */
    else if(!strcmp(cmd->elems_v, "_"))
    {cl->currsel = cl->bufsel;}

        /* if there's a selection cmd sets a new current selection */
    else
    {
        get_nums(cmd, cl, t, exit_code);
        cl->cmds[cl->cmds_c].cmd_opt = SEL;
        cl->cmds[cl->cmds_c].proc_opt = CHANGESEL;
#ifdef SELECT
        printf("\nINIT_N_WSPASED_CMDS\n     %d  new selection [%d,%d,%d,%d]\n", __LINE__,
               cl->cmds[cl->currsel].row_1,
               cl->cmds[cl->currsel].col_1,
               cl->cmds[cl->currsel].row_2,
               cl->cmds[cl->currsel].col_2);
#endif
    }
    if(cl->cmds[cl->cmds_c].proc_opt >= MIN && cl->cmds[cl->cmds_c].proc_opt <= MAX)
    {cl->cmds[cl->cmds_c].cmd_opt = SEL;}

    else if(cl->cmds[cl->cmds_c].proc_opt >= SETTMP && cl->cmds[cl->cmds_c].proc_opt <= DCOL)
    {cl->cmds[cl->cmds_c].cmd_opt = PRC;}

    FREE(currcom)
    CHECK_EXIT_CODE
}




/* increase the number of given commands, clear the line where the previous command was */
void prep_for_next_cmd(carr_t *cmd, int *cmds_c, const int *pos, const int *arglen)
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
    printf("\nPROCESS_CMD\n     %d  command opt = %s command = %s \n",
           __LINE__, print_cmd_opt(cl->cmds[cl->cmds_c].cmd_opt), print_opt(cl->cmds[cl->cmds_c].proc_opt));
#endif
    /* expand tab to fit the selection */
    expand_tab(t, cl->cmds[cl->cmds_c].row_2, cl->cmds[cl->cmds_c].col_2, exit_code);

    switch(cl->cmds[cl->cmds_c].cmd_opt)
    {
        case SEL: /* selection is already set in the functinon */
            process_sel(cl,t, exit_code);
#ifdef CMDS
            printf("\n     %d cursel: [%d,%d,%d,%d]\n", __LINE__, cl->cmds[cl->currsel].row_1,
                   cl->cmds[cl->currsel].col_1,
                   cl->cmds[cl->currsel].row_2,
                   cl->cmds[cl->currsel].col_2
            );
#endif
            break;

        /* call a process_table() function that caalls one of inicialized functions for processing the table  */
        case PRC:
            process_table(cl, t, exit_code);
            break;

        case NOPT:
            *exit_code = ARG_UNRECARG_ERR;
            CHECK_EXIT_CODE
    }
}

void create_cmd(cmd_t *cmd)
{
    cmd->col_2 = 0;
    cmd->col_1 = 0;
    cmd->row_1 = 0;
    cmd->row_2 = 0;
    cmd->proc_opt = NOTH;
    cmd->iscolsel = false;
    cmd->isinit = false;
    cmd->cmd_opt = NOPT;
    memset(cmd->pttrn, 0, PTRNLEN);
}


void init_cmd(carr_t *cmd, tab_t *t, cl_t *cl, int *exit_code)
{
    /* fill everyrhing with 0s */
    create_cmd(&cl->cmds[cl->cmds_c]);

    cell_trim(cmd, exit_code); /* trim a command */ // TODO seems it is unneccesary
    CHECK_EXIT_CODE

#ifdef SELECT
    printf("\nINIT_CMD\n     %d  cmd = \"%s\", cmd number = %d\n", __LINE__, cmd->elems_v, cl->cmds_c);
#endif

    /* edit structure of the table or change current selection */
    if(strchr(cmd->elems_v, ' ') == NULL)
    {
        init_n_wspased_cmd(cmd, cl, t, exit_code);
    }
        /* init and call data processing functions or process temp variables */
    else
    {
        init_wspased_cmd(cmd, cl, exit_code);
    }
    CHECK_EXIT_CODE
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
    printf("\nINIT_CMDS\n");
#endif
    //region variables
    int arglen = (int)strlen(arg);
    bool quoted = false;
    bool backslashed = false;

    /* firsst cell selection command is on the position 0,
     * also it is neccesarry to init cmds count */
    cl->cellsel = (cl->cmds_c = 0);

    /* set a selection to the first cell(by default) */
    set_sel(cl,1, 1, 1, 1, exit_code, RCCELL);

    /* current command(0) is a selection */
    cl->cmds[cl->cmds_c].cmd_opt = SEL;
    cl->cmds[cl->cellsel].iscolsel = true;

    /* first cmd is a 1 1(default) selection  */
    cl->cmds_c = 1;
    cl->temps_c = 0;

    /* create all temp selection commands */
    for(int i = 0; i < NUM_TMP_SELS; i++)
    {
        create_cmd(&cl->tmpsel[i]);
    }
    //endregion

    /* walk through other tokens */
    for(int p = 0; p < arglen; p++)
    {
        if((arg[p] == ';' && !backslashed && !quoted) || p == arglen - 1)
        {
            if(arg[p] != ';' && arg[p] != ']')
            {
                a_carr(cmd, arg[p], exit_code);
            }

            /* initialize a cmd */
            init_cmd(cmd, t, cl, exit_code);
            CHECK_EXIT_CODE

            /* process an initialized cmd change the exit_code */
            process_cmd(cl, t, exit_code);
            CHECK_EXIT_CODE

            prep_for_next_cmd(cmd, &cl->cmds_c, &p, &arglen);
        }
        /* if there is a backslash in the command */
        else if(arg[p] == '\\')
        {
            /* dont add a backslash if it is already escaped */
            if(backslashed)
            {
                a_carr(cmd, arg[p], exit_code);
                a_carr(cmd, arg[p], exit_code);
            }
            NEG(backslashed)
        }

        else if(arg[p] == '\"')
        {
            if(backslashed)
            {
                a_carr(cmd, '\\', exit_code);
                a_carr(cmd, arg[p], exit_code);
                NEG(backslashed)
            }else{ NEG(quoted)}
        }

        else if((arg[p] == '[' || arg[p] == ']'))
        {
            if(backslashed)
            {
                a_carr(cmd, arg[p], exit_code);
                NEG(backslashed);
            }
            if(quoted)
            {
                a_carr(cmd, arg[p], exit_code);
            }
        }
        else
        {
            if(backslashed){NEG(backslashed)}
            a_carr(cmd, arg[p], exit_code);
        }

    }
}

/**
 * Initializes an array of entered separators(DELIM)
 *
 * @param argc Number of cmdline arguments
 * @param argv An array with cmdline arguments
 * @param exit_code exit code to change if an error occurred
 */
void init_separators(const int argc, const char **argv, cl_t *cl, int *exit_code)
{
    //region variables
    cl->seps.len = 0;
    carr_ctor(&cl->seps, MEMBLOCK, exit_code);
    CHECK_EXIT_CODE
    int k = 0;
    //endregion

    /* if there is only name, functions and filename */
    if(argc == 3 && strcmp(argv[1], "-d") != 0)
    {
        a_carr(&cl->seps, ' ', exit_code);
        cl->defaultsep = true;
        CHECK_EXIT_CODE
    }

        /* if user entered -d flag and there are function names and filename in the cmdline */
    else if(argc == 5 && !strcmp(argv[1], "-d"))
    {
        cl->defaultsep = false;
        while(argv[2][k])
        {
            /* checks if the user has not entered characters that will lead to undefined program behavior */
            if(argv[2][k] == '\\' || argv[2][k] == '\"' || argv[2][k] == '\'')
            {
                *exit_code = W_SEPARATORS_ERR;
                CHECK_EXIT_CODE
            }
            set_add_item(&(cl->seps), argv[2][k], exit_code);
            CHECK_EXIT_CODE
            k++;
        }
    } else
    {
        printf("%d %d\n", __LINE__, argc);
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
    FREE(cmd.elems_v)
    CHECK_EXIT_CODE
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

void prepare_tab_bef_printing(carr_t *seps, tab_t *t, int *exit_code)
{
    /* trim table before printing */ //
    tab_trim_bef_printing(t, exit_code);
    CHECK_EXIT_CODE

    /* quote the cells if there are cells with escaped separators in the table */
    tab_add_quotermakrs(seps, t, exit_code);
    CHECK_EXIT_CODE
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
    tab_t t;
    cl_t cl;
    //endregion

    /* initialize separators from cmdline */
    init_separators(argc, argv, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* adds table to the structure */
    get_table(&argc, argv, &t, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* parse cmdline arguments and process table for them */
    parse_cl_proc_tab(&argc, argv, &t, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* beautify the table */
    prepare_tab_bef_printing(&cl.seps, &t,&exit_code);
    
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
