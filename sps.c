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


/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */
#define CHECK_EXIT_CODE_IN_RUN_PROGRAM if(exit_code > 0)\
{\
    print_error_message(&exit_code);\
    clear_data(&t, &cl);\
    return exit_code;\
}

/* checks exit code in a void function */
#define CHECK_EXIT_CODE if(*exit_code > 0){ return; }

/* check for a memory error */
#define CHECK_ALLOC_ERR(arr) if((arr) == NULL)\
{\
    *exit_code = W_ALLOCATING_ERR;\
    CHECK_EXIT_CODE\
}

/* checks exit code in get_row() and get_col() functionss */
#define GRC_CHECK_EXIT_CODE if(*exit_code > 0)\
{\
    return GRCERR;\
}

/* check for a memory error get_row() and get_col() functionss */
#define GRC_CHECK_ALLOC_ERR(arr) if((arr) == NULL)\
{\
    *exit_code = W_ALLOCATING_ERR;\
    GRC_CHECK_EXIT_CODE\
}

#define MEMBLOCK     10 /* allocation step */
#define PTRNLEN      1001
#define NUM_TMP_SELS 10 /* max number of temo selections */

#define NEG(n) ((n) = !(n)); /* negation of bool value */

#define CHECK_TAB_DELETED if(t->deleted){NEG(t->deleted)}
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
    RCRC,
    RCCELL,
    RCEXCEPT,
    NOSELOPT
} sel_opt;

/* option if return value of getcell function */
typedef enum
{
    /* "bad row". means that the column is terminated by EOF character,
     * which, according to the project specification, means
     * that this row is not considered a row
     * */
    FEND,

    /* "next row". The cell is terminated by the \n character
     * and program must handle next row
     * */
    NROW,

    /* Means there was an error with memory allocating or with input file */
    GRCERR,

    /* "next cell". Means there will be next cell,
     * because current cell is terminated by a separator
     * */
    NCELL

} gcr_retval_opt; /*"get column or row" in oldspeak */


enum erorrs
{
    W_ALLOCATING_ERR = 1, /* error caused by a 'memory' function */
    W_SEPARATORS_ERR,     /* wrong separators have been entered  */
    VAL_UNSUPARG_ERR,     /* unsupported number of arguments     */
    VAL_UNSUPCMD_ERR,     /* wrong arguments in the cmdline  */
    ARG_UNRECARG_ERR,     /* unrecognized argument               */
    NO_SUCH_FILE_ERR,     /* no file with the entered name exists */
    LEN_UNSUPCMD_ERR,     /* unsupported len of the cmd   */
    UNREC_CMD_ERR,        /* unrecognized command */
    UNDEF_TMPCMD_ERR,     /* indefined temp command */
    BAD_INPUTFILE_ERR     /* if quotes dont match in the file or newlin/EOF is backslashed */
};

enum argpos
{
    /* position with no delim means first argument after name of the program */
    POSNDEL = 1,

    /* position with delim means first argument after delim string */
    POSWDEL = 3
};

/* enum with all commands */
typedef enum
{
    NOTH,
    CHANGESEL,

    DEF, USE, INC, FIND,
    MIN, MAX, SETTMP, USETMP,

    SET, CLEAR, IROW, AROW, DROW, ICOL, ACOL, DCOL,
    SWAP, AVG, SUM, COUNT, LEN,
} opts_t;

/* enum with comand options */
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
    if(t->deleted || t->isempty)
        return;

    for(int row = 0; row <= t->row_c; row++)
    {
        for(int col = 0; col <= t->rows_v[row].cols_c; col++)
        {
            if(col)
            {fputc(seps->elems_v[0], ptr);}

            for(int i = 0; i < t->rows_v[row].cols_v[col].elems_c; i++)
                fputc(t->rows_v[row].cols_v[col].elems_v[i], ptr);

        }
        putc('\n', ptr);
    }
}

//region FREE MEM

void free_row(row_t *r)
{
    for(int col = r->len - 1; col >= 0; col--)
    {
        free(r->cols_v[col].elems_v);
    }
    free(r->cols_v);
}

/* frees all memory allocated by a table_t structure s */
void free_table(tab_t *t)
{
    for(int row = t->len - 1; row >= 0; row--)
    {
        free_row(&t->rows_v[row]);
    }
    free(t->rows_v);
}

/* frees all memory allocated by a cl_t structure s */
void free_cl(cl_t *s)
{
    free(s->seps.elems_v);
    if(s->ptr)
        fclose(s->ptr);
}

/* clears all dynamic memroy that has been allocated and closes the file */
void clear_data(tab_t *t, cl_t *cl)
{
    free_table(t);
    free_cl(cl);
}
//endregion

//region ARRAYS WITH CHARS FUNCS

/* set true to the cell if it has only number inside */
void iscellnum(carr_t *cell)
{
    char *junk;
    cell->isnum = true;
    strtod(cell->elems_v, &junk);
    if(junk[0])
    {cell->isnum = false;}
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
/* cleat an array with chars inside carr_t structure */
void carr_clear(carr_t *carr)
{
    /* there is no need to clear an empty array */
    carr->isempty = true;
    carr->isnum = false;
    memset(carr->elems_v, 0, carr->len - 1);
    carr->elems_c = 0;
}

/* allocate memory for an array of chars in the carr_t structure */
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
 * Allocate new rows if r2 > nomber of rows in the table
 * @param t
 * @param r2
 * @param c2
 * @param exit_code
 */
void add_rows(tab_t *t, int r2, int *exit_code)
{
    if(r2 > t->len)
    {
        /* allocate new memory for rows */
        t->rows_v = (row_t *)realloc(t->rows_v, r2 * sizeof(row_t));
        CHECK_ALLOC_ERR(t->rows_v)

        t->len = r2;

        /* allocate new memory for columns */
        for(int r = t->row_c + 1; r < t->len; r++)
        {
            row_ctor(&t->rows_v[r], t->col_c + 1, MEMBLOCK, exit_code);
            CHECK_EXIT_CODE
            t->rows_v[r].cols_c = t->col_c;
        }
        t->row_c = t->len - 1;
    }
}

/**
 * Reallocate each row for a new size c2 of c2 > number of columns in the table
 * @param t
 * @param c2
 * @param exit_code
 */
void add_cols(tab_t *t, int c2, int *exit_code)
{
    for(int r = 0; r < t->len; r++)
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

            //	t->col_c = t->rows_v[r].len - 1; /* now all rows have len of max col len */
        }
    }
}

/**
 *  Expand the table
 *
 * @param r2 Lower row
 * @param c2 Right column
 * @param exit_code if memore was not allocated
 */
void expand_tab(tab_t *t, int r2, int c2, int *exit_code)
{

    t->deleted = false;

    /* expand_rows */
    add_rows(t, r2, exit_code);
    CHECK_EXIT_CODE

    add_cols(t, c2, exit_code);
    CHECK_EXIT_CODE
}

/* trims an array of characters */
void cell_trim(carr_t *arr, int *exit_code)
{
    if(arr->len >= arr->elems_c + 1)
    {
        /* reallocate for elements and terminating 0 */
        arr->elems_v = (char *)realloc(arr->elems_v, (arr->elems_c + 1) * sizeof(char));
        CHECK_ALLOC_ERR(arr->elems_v)

        arr->len = (arr->elems_c) ? arr->elems_c + 1 : 1;
    }
}

void row_trim(row_t *row, int siz, int *exit_code, rtrim_opt opt)
{
    /* trim all cells in the row */
    if(row->len > siz)
    {
        for(int i = row->len - 1; i >= siz; i--)
        {
            free(row->cols_v[i].elems_v);
        }
        row->cols_v = (carr_t *)realloc(row->cols_v, siz * sizeof(carr_t));
        CHECK_ALLOC_ERR(row->cols_v)
        row->len = siz;
        row->cols_c = siz - 1;
    } else if(row->len < siz)
    {
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

    return r->len; /* means the whole table is not empty */
}

/* find the longest unempty row in the table */
int find_max_unemp(tab_t *t)
{
    int max_unemp = 0; /* prevent memory error if the whole table is empty */
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
    if(!max_unemp) /* means all columns in the all rows are empty */
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
void quote_cell(carr_t *seps, carr_t *cell, int *exit_code)
{
    /* if there is a separator in the cell */
    for(int j = 0; j <= seps->elems_c; j++)
    {
        /* if there will be troubles with project evaluation because of the quotation marks and backslashes show the line below */
        //if((strchr(cell->elems_v, seps->elems_v[j]) != NULL && seps->elems_v[j]) || strchr(cell->elems_v, '\\') != NULL || strchr(cell->elems_v, '\"') != NULL)
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
    /* went through all rows */
    for(int r = 0; r < t->len; r++)
    {
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

    if(max_unempt)
    {
        for(int r = 0; r < t->len; r++)
        {
            row_trim(&t->rows_v[r], max_unempt, exit_code, NCOLS);
        }
    }
}


/* trims a table by reallocating rows and cols */
void table_trim(tab_t *t, int *exit_code)
{
    int row = 0;
    int maxlen = 0;

    if(t->row_c + 1 < t->len)
    {
        /* free unused rows that have been alloacated */
        for(row = t->len - 1; row > t->row_c; row--)
        {
            for(int col = t->rows_v[row].len - 1; col >= 0; col--)
                free(t->rows_v[row].cols_v[col].elems_v);
            free(t->rows_v[row].cols_v);
        }

        t->len = t->row_c + 1;
        t->rows_v = (row_t *)realloc(t->rows_v, (t->len) * sizeof(row_t));
        CHECK_ALLOC_ERR(t->rows_v)
    }

    /* trim each row and find max length of the row in the table */
    for(row = 0; row < t->len; row++)
    {
        row_trim(&t->rows_v[row], t->rows_v[row].cols_c + 1, exit_code, WCOLS);
        CHECK_EXIT_CODE

        if(!row || maxlen < t->rows_v[row].cols_c)
        {
            maxlen = t->rows_v[row].cols_c + 1;
        }
    }

    /* reallocate all rows with new size of maxlen */
    for(row = 0; row < t->len; row++)
    {
        row_trim(&t->rows_v[row], maxlen, exit_code, NCOLS);
        CHECK_EXIT_CODE
    }

    t->col_c = maxlen - 1;
}
//endregion


//region GETTERS

/* check if selection meets the conditions and set it as a current selction */
void set_sel(cl_t *cl, int row1, int col1, int row2, int col2, int *exit_code, sel_opt opt)
{
    if(opt == RCRC)
    {
        /* check if the given selectinon meets the conditions */
        if(row2 < row1 || col2 < col1 ||
           row1 < cl->cmds[cl->cellsel].row_1 ||
           col1 < cl->cmds[cl->cellsel].col_1 ||
           row2 < cl->cmds[cl->cellsel].row_2 ||
           col2 < cl->cmds[cl->cellsel].col_2)
        {
            *exit_code = VAL_UNSUPARG_ERR;
        }
    }
    if(opt == RCCELL)
    {
        cl->cellsel = cl->cmds_c;
    } else if(opt == NOSELOPT)
    {
        *exit_code = VAL_UNSUPARG_ERR;
    }
    CHECK_EXIT_CODE

    /* change old current selection to the new current selection */
    cl->currsel = cl->cmds_c;

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
                } else  /* in case of row selection */
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
            {*exit_code = W_SEPARATORS_ERR;}

            CHECK_EXIT_CODE
        } else if(ptr[0] != 0)
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
        if(!uscore)
        {
            opt = RCEXCEPT;
        } else opt = RCCELL;
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
    } else
    {
        *exit_code = ARG_UNRECARG_ERR;
    }
    CHECK_EXIT_CODE

    set_sel(cl, nums[0], nums[1], nums[2], nums[3], exit_code, opt);
    CHECK_EXIT_CODE
}

/**
 *  Add a cell to the row
 * @return see abowe in get_retval_opt enum
 */
gcr_retval_opt get_cell(carr_t *col, cl_t *cl, int *exit_code)
{
    char buff_c;
    bool quoted = false;
    bool backslashed = false;

    for(col->elems_c = 0;;)
    {
        buff_c = (char)fgetc(cl->ptr);

        if(buff_c == '\\')
        {
            if(backslashed)
            {
                a_carr(col, buff_c, exit_code);
                a_carr(col, buff_c, exit_code);
            }
            NEG(backslashed)
        } else if(buff_c == '\"')
        {
            if(backslashed)
            {
                a_carr(col, '\\', exit_code);
                a_carr(col, buff_c, exit_code);
                NEG(backslashed)
            } else
            {NEG(quoted)}
        }
            /* means next row is reached */
        else if(buff_c == '\n')
        {
            /* newline cannot be backslashed or quoted */
            if(backslashed || quoted)
            {*exit_code = BAD_INPUTFILE_ERR;}
            return NROW;
        }

            /* means EOF is reached */
        else if(buff_c == EOF)
        {
            /* EOF cannot be backshashed or quoted  */
            if(backslashed || quoted)
            {*exit_code = BAD_INPUTFILE_ERR;}
            return FEND;
        }

            /* if the char is a separator and it is not quoted */
        else if(strchr(cl->seps.elems_v, buff_c) != NULL && !quoted && !backslashed)
        {
            return NCELL;
        } else
        {
            if(backslashed)
            {NEG(backslashed)}
            a_carr(col, buff_c, exit_code);
        }
    }
}

/* gets a row from the file and adds it to the table */
gcr_retval_opt get_row(row_t *row, cl_t *cl, int *exit_code)
{
    gcr_retval_opt retval = 0;

    for(row->cols_c = 0; /* hope on FEND and NROW */; row->cols_c++)
    {
        if(row->cols_c == row->len)
        {
            row->len += MEMBLOCK;
            row->cols_v = (carr_t *)realloc(row->cols_v, row->len * sizeof(carr_t));
            GRC_CHECK_ALLOC_ERR(row->cols_v)

            for(int c = row->len - 1; c >= row->cols_c; c--)
            {
                /* create a col */
                carr_ctor(&row->cols_v[c], MEMBLOCK, exit_code);
                GRC_CHECK_EXIT_CODE
            }
        }

        retval = get_cell(&row->cols_v[row->cols_c], cl, exit_code);
        GRC_CHECK_EXIT_CODE

        /* check if the cell is a number and set true or false to the cell */
        iscellnum(&row->cols_v[row->cols_c]);

        /* if functions get_cell() returns everything except the NCELL */
        if(retval <= GRCERR)
        {
            return retval;
        }
    }
}

/* reallocate a table if there is a need to */
void check_for_realloc_table(tab_t *t, int *exit_code)
{
    if(t->row_c == t->len)
    {
        t->len += MEMBLOCK;
        t->rows_v = (row_t *)realloc(t->rows_v, (t->len) * sizeof(row_t));
        CHECK_ALLOC_ERR(t->rows_v)

        for(int rc = t->len - 1; rc >= t->row_c; rc--)
        {
            /* create a row */
            row_ctor(&t->rows_v[rc], MEMBLOCK, MEMBLOCK, exit_code);
            CHECK_EXIT_CODE
        }
    }
}

/* write table from file to the table structure (tab_t)*/
void write_tab_to_struct(cl_t *cl, tab_t *t, int *exit_code)
{
    for(t->row_c = 0;;)
    {
        gcr_retval_opt retval;

        /* realloc the table to the new size  */
        check_for_realloc_table(t, exit_code);
        CHECK_EXIT_CODE

        retval = get_row(&t->rows_v[t->row_c], cl, exit_code);

        /* if the EOF reached delete the last row */
        if(retval == FEND)
        {
            /* decrease number of rows because it represents position of the last element in the array */
            t->row_c--;
            break;
        }
            /* if there are other rows continue file processing */
        else if(retval == NROW)
        {
            t->row_c++;
        }
        CHECK_EXIT_CODE
        t->isempty = false;
    }

    /* it means file was empty */
    if(t->row_c == -1)
    {
        t->row_c = 0;
    }
}

void get_table(const int argc, const char **argv, tab_t *t, cl_t *cl, int *exit_code)
{
    if((cl->ptr = fopen(argv[argc - 1], "r")) == NULL)
    {
        *exit_code = NO_SUCH_FILE_ERR;
        return;
    }
        /* if file has been opened successfully */
    else
    {
        /* write the table to the structure */
        write_tab_to_struct(cl, t, exit_code);
        CHECK_EXIT_CODE

        /* reopen the file to start overwiting */
        freopen(argv[argc - 1], "w", cl->ptr);

        table_trim(t, exit_code);
        CHECK_EXIT_CODE
    }
}
//endregion


//region functions

/**
 *  The project specification says that this function only works with one cell.
 *   I could misunderstand the interpretation of the task, so I imlemented set_second function
 *   that will insert the pattern into all cells from the selection instead of only one
 *
 *  "set STR - nastaví hodnotu buňky na řetězec STR. Řetězec STR může být ohraničen uvozovkami a může obsahovat
 *  speciální znaky uvozené lomítkem (viz formát tabulky)"
 */
void set_f(tab_t *t, cl_t *cl, int *exit_code)
{
#define rto cl->cmds[cl->currsel].row_2
#define cto cl->cmds[cl->currsel].col_2
#define rfr cl->cmds[cl->currsel].row_1
#define cfr cl->cmds[cl->currsel].col_1
#define str_set cl->cmds[cl->cmds_c].pttrn

    expand_tab(t, rto, cto, exit_code);
    CHECK_EXIT_CODE
    int len = 0;

    len = (int)strlen(str_set);
    /* go through al selected rows */
    for(int r = rfr - 1; r < rto; r++)
    {
        for(int c = cfr - 1; c < cto; c++)
        {
            carr_clear(&t->rows_v[r].cols_v[c]); /* clear a cell */
            for(int p = 0; p < len; p++)
            {
                a_carr(&t->rows_v[r].cols_v[c], str_set[p], exit_code);
                CHECK_EXIT_CODE
            }
        }
    }
#undef rto
#undef cto
#undef rfr
#undef cff
}


/**
 * sets the current cell selection to the temporary variable _
 * (only which cells are selected, not their contents)
 */
void set_tmp_f(cl_t *cl)
{
    cl->bufsel = cl->currsel; /* save the current selection to the buffer selection */
}

/* If there was no window selection, aoutomatically use the first selection [1,1]*/
void use_tmp_f(cl_t *cl)
{
    cl->currsel = cl->bufsel;
}

/**
 * define a tepm selection
 */
void def_f(cl_t *cl)
{
#define pos cl->cmds[cl->cmds_c].row_1

    /* copy a current cell selection to the command */
    cl->tmpsel[pos].row_2 = (cl->tmpsel[pos].row_1 = cl->cmds[cl->cellsel].row_1);
    cl->tmpsel[pos].col_2 = (cl->tmpsel[pos].col_1 = cl->cmds[cl->cellsel].col_1);
    cl->temps_c++; /* finally increase number of temp selections */
    cl->tmpsel[pos].isinit = true; /* init the command*/

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
#define pos_tmp cl->cmds[cl->cmds_c].row_1 /* shows position in array of temp selections */
#define r_tmp cl->tmpsel[pos_tmp].row_1 /*    row from temp selection                    */
#define c_tmp cl->tmpsel[pos_tmp].col_1 /*    column from temp selection                 */

#define r1 cl->cmds[cl->currsel].row_1 /*      row from cell selection                    */
#define c1 cl->cmds[cl->currsel].col_1 /*      column from cell selection                 */
#define r2 cl->cmds[cl->currsel].row_2 /*      row from cell selection                    */
#define c2 cl->cmds[cl->currsel].col_2 /*      column from cell selection                 */

    /* check if the command is initialized */
    if(cl->tmpsel[pos_tmp].isinit)
    {
        /* if sizes of table have been changed between "def _" and "use _" commands there is need to expand the table */
        expand_tab(t, r_tmp, c_tmp, exit_code);
        CHECK_EXIT_CODE

        for(int r = r1 - 1; r < r2; r++)
        {
            for(int c = c1 - 1; c < c2; c++)
            {
                if(r != r_tmp - 1 || c != c_tmp - 1) /* to prevent overwriting */
                {
                    carr_clear(&t->rows_v[r].cols_v[c]);
                    for(int i = 0; i < t->rows_v[r_tmp - 1].cols_v[c_tmp - 1].len - 1; i++)
                    {
                        a_carr(&t->rows_v[r].cols_v[c], t->rows_v[r_tmp - 1].cols_v[c_tmp - 1].elems_v[i],
                               exit_code);
                        CHECK_EXIT_CODE
                    }
                }

            }
        }

    } else *exit_code = UNDEF_TMPCMD_ERR;
    CHECK_EXIT_CODE

#undef c2
#undef r2
#undef c1
#undef r1
#undef c_tmp
#undef r_tmp
#undef pos_tmp
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
        char tmpresult[PTRNLEN] = {0}; /* create a string where result will bewritten */
        /* if cell contains a number increase it by 1*/
        if(t->rows_v[row - 1].cols_v[col - 1].isnum)
        {
            /* extract number from the cell*/
            float num_in_cell = (float)strtod(t->rows_v[row - 1].cols_v[col - 1].elems_v, NULL);
            num_in_cell++; /* increase the number by 1 */
            sprintf(tmpresult, "%g", num_in_cell); /* add a result to the temp string with result */
        }
            /* means temporary variable does not contain a number set 1 in the cell */
        else
        {
            tmpresult[0] = '1';
        }


        int numlen = (int)strlen(tmpresult);
        carr_clear(&t->rows_v[row - 1].cols_v[col - 1]);
        for(int i = 0; i < numlen; i++)
        {
            a_carr(&t->rows_v[row - 1].cols_v[col - 1], tmpresult[i], exit_code);
            CHECK_EXIT_CODE
        }

        t->rows_v[row - 1].cols_v[col - 1].isnum = true;

    } else *exit_code = UNDEF_TMPCMD_ERR;
    CHECK_EXIT_CODE
#undef col
#undef row
#undef pos
}

void carr_overwrite(carr_t *dst, char *src, int *exit_code)
{
    carr_clear(dst);
    int len = (int)strlen(src);

    for(int i = 0; i < len; i++)
    {
        a_carr(dst, src[i], exit_code);
        CHECK_EXIT_CODE
    }
}

/**
 * len [R, C] - stores the string length of the currently selected cell in the cell on row R and column C.
 */
void len_f(cl_t *cl, tab_t *t, int *exit_code)
{
#define r cl->cmds[cl->cellsel].row_1
#define c cl->cmds[cl->cellsel].col_1
#define row_topast (cl->cmds[cl->cmds_c].row_1 - 1)
#define col_topast (cl->cmds[cl->cmds_c].col_1 - 1)

    sprintf(cl->cmds[cl->cmds_c].pttrn, "%d", (int)strlen(t->rows_v[r - 1].cols_v[c - 1].elems_v));
    carr_overwrite(&t->rows_v[row_topast].cols_v[col_topast], cl->cmds[cl->cmds_c].pttrn, exit_code);
    CHECK_EXIT_CODE

#undef row_topast
#undef col_topast
#undef r
#undef c
}

void count_f(cl_t *cl, tab_t *t, int *exit_code)
{
#define r1 cl->cmds[cl->currsel].row_1
#define r2 cl->cmds[cl->currsel].row_2
#define c1 cl->cmds[cl->currsel].col_1
#define c2 cl->cmds[cl->currsel].col_2
#define row_topast (cl->cmds[cl->cmds_c].row_1 - 1)
#define col_topast (cl->cmds[cl->cmds_c].col_1 - 1)
    int nempties = 0;

    for(int r = r1 - 1; r < r2; r++)
    {
        for(int c = c1 - 1; c < c2; c++)
        {
            if(!t->rows_v[r].cols_v[c].isempty)
            {
                nempties++;
            }
        }
    }

    sprintf(cl->cmds[cl->cmds_c].pttrn, "%d", nempties);
    carr_overwrite(&t->rows_v[row_topast].cols_v[col_topast], cl->cmds[cl->cmds_c].pttrn, exit_code);
    CHECK_EXIT_CODE
#undef row_topast
#undef col_topast
#undef r1
#undef r2
#undef c1
#undef c2
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
#define row_topast (cl->cmds[cl->cmds_c].row_1 - 1)
#define col_topast (cl->cmds[cl->cmds_c].col_1 - 1)
#define r_fr cl->cmds[cl->currsel].row_1
#define r_to cl->cmds[cl->currsel].row_2
#define c_fr cl->cmds[cl->currsel].col_1
#define c_to cl->cmds[cl->currsel].col_2

    int numcels = 0;
    double result = 0;
    char *junk = NULL;
    double tmp_res = 0;

    /* it could be done better but unfortunatelly i had no time at all */
    for(int r = r_fr - 1; r < r_to; r++)
    {
        for(int c = c_fr - 1; c < c_to; c++)
        {
            tmp_res = strtod(t->rows_v[r].cols_v[c].elems_v, &junk);

            /* if the cell is not empty and there was no string iin the cell*/
            if(!junk[0] && t->rows_v[r].cols_v[c].elems_v[0])
            {
                result += tmp_res;
                numcels++;
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
    } else
    {
        cl->cmds[cl->cmds_c].pttrn[0] = '0';
    }

    carr_overwrite(&t->rows_v[row_topast].cols_v[col_topast], cl->cmds[cl->cmds_c].pttrn, exit_code);
    CHECK_EXIT_CODE
#undef r_fr
#undef r_to
#undef c_fr
#undef c_to
#undef row_topast
#undef col_topast
}


/**
 * swap [R,C] - swaps the contents of the selected cell with the cell from the last cell selection
 */
void swap_f(cl_t *cl, tab_t *t)
{
    /* create macros to use less mempry and to make the code more readable */
#define r (cl->cmds[cl->cellsel].row_1 - 1)
#define c (cl->cmds[cl->cellsel].col_1 - 1)
#define r1 (cl->cmds[cl->cmds_c].row_1 - 1)
#define c1 (cl->cmds[cl->cmds_c].col_1 - 1)

    if(t->deleted) return;
    char *tmp = t->rows_v[r].cols_v[c].elems_v;
    int tempum = t->rows_v[r].cols_v[c].len;
    //int elems_c = t->rows_v[r].cols_v[c].elems_c;

    /* copy 2nd row to the first row */
    t->rows_v[r].cols_v[c].elems_v = t->rows_v[r1].cols_v[c1].elems_v;
    t->rows_v[r].cols_v[c].len = t->rows_v[r1].cols_v[c1].len;
    t->rows_v[r1].cols_v[c1].len = tempum;
    tempum = t->rows_v[r].cols_v[c].elems_c;
    t->rows_v[r].cols_v[c].elems_c = t->rows_v[r1].cols_v[c1].elems_c;
    t->rows_v[r1].cols_v[c1].elems_c = tempum;

    t->rows_v[r1].cols_v[c1].elems_v = tmp;

#undef c1
#undef r1
#undef c
#undef r
}


void min_max_f(cl_t *cl, tab_t *t, int *exit_code, int opt)
{
    if(t->deleted) return;
    /* start */
    int r = cl->cmds[cl->currsel].row_1 - 1;

    int targ_row = -1, targ_col = -1;
    double mval = 0, mtempval = 0;
    bool found = false;

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
        cl->cellsel = cl->cmds_c;
        set_sel(cl, targ_row + 1, targ_col + 1, targ_row + 1, targ_col + 1, exit_code, RCCELL);
    }
}

/* in an existing cell selection, selects the first cell whose value contains the substring STR */
void find_f(cl_t *cl, tab_t *t, int *exit_code)
{
    if(t->deleted) return;
    int r = cl->cmds[cl->currsel].row_1 - 1;

    /* walk through all selected rows and columns */
    for(; r < cl->cmds[cl->currsel].row_2; r++)
    {
        for(int c = cl->cmds[cl->currsel].col_1 - 1; c < cl->cmds[cl->currsel].col_2; c++)
        {
            if(strstr(t->rows_v[r].cols_v[c].elems_v, cl->cmds[cl->cmds_c].pttrn) != NULL)
            {
                cl->cellsel = cl->cmds_c;
                set_sel(cl, r + 1, c + 1, r + 1, c + 1, exit_code, RCCELL);
                return;
            }
        }
    }
}

/**
 * Delete content of the cells in given area indicated by parameters r1, c1, r2, c2
 *
 * @param r1 "row from"
 * @param c1 "column from"
 * @param r2 "row to"
 * @param c2 "column to"
 * @param t table itself
 */
void clear_f(int r1, int c1, int r2, int c2, tab_t *t)
{
#ifdef CMDS
    printf("\nCLEAR\n     %d  currsel: [%d,%d,%d,%d]\n",__LINE__, r1, c1, r2, c2);
#endif

    for(int row_fr = r1 - 1; row_fr < r2; row_fr++)
    {
        for(int col_fr = c1 - 1; col_fr < c2; col_fr++)
        {
            carr_clear(&t->rows_v[row_fr].cols_v[col_fr]);
        }
    }
}

/* inserts one blank row to the left/right of the selected cells */
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

    /* create a row */
    row_ctor(&t->rows_v[upper_b], t->col_c + 1, MEMBLOCK, exit_code);
    t->rows_v[upper_b].cols_c = t->col_c;
    CHECK_EXIT_CODE
}

/* add or insert a column before or after the selection*/
void icol_acol_f(cl_t *cl, tab_t *t, int *exit_code, int opt)
{
    int left_b = 0; /* declare a left border */

    CHECK_TAB_DELETED

    /* initialize a left border */
    if(opt == ICOL)
    {
        left_b = cl->cmds[cl->currsel].col_1 - 1;

    } else if(opt == ACOL)
    {
        left_b = cl->cmds[cl->currsel].col_2;
    } else return;

    /* walk through each row */
    for(int r = 0; r < t->len; r++)
    {
        t->rows_v[r].len++;

        /* increase size of each row */

        t->rows_v[r].cols_v = (carr_t *)realloc(t->rows_v[r].cols_v, (t->rows_v[r].len) * sizeof(carr_t));

        /* move all cols right */
        for(int c = t->rows_v[r].len - 1; c > left_b; c--)
        {
            t->rows_v[r].cols_v[c] = t->rows_v[r].cols_v[c - 1];
        }

        /* insert a column itself */
        carr_ctor(&t->rows_v[r].cols_v[left_b], MEMBLOCK, exit_code);
        CHECK_EXIT_CODE
        t->rows_v[r].cols_c++;
    }
    t->col_c++;
}

void delete_table(tab_t *t)
{
    clear_f(0, 0, t->len, t->col_c + 1, t);
    t->deleted = true;
}

void drow_f(cl_t *cl, tab_t *t, int *exit_code)
{
    int from = cl->cmds[cl->currsel].row_1;
    int to = cl->cmds[cl->currsel].row_2;
    /* if the whole table is about to be deleted  */
    if(from == 1 && to >= t->len)
    {
        delete_table(t);
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

    for(int r = from - 1; r < to; r++)
    {
        for(int c = t->rows_v[r].len - 1; c >= 0; c--)
        {
            free(t->rows_v[r].cols_v[c].elems_v);
        }
        free(t->rows_v[r].cols_v);
    }

    for(int r = from - 1; r + diff < t->len; r++)
    {
        t->rows_v[r] = t->rows_v[r + diff];
    }
    t->row_c -= diff;
    t->len -= diff;

    /* re-allocate memory */
    t->rows_v = (row_t *)realloc(t->rows_v, t->len * sizeof(row_t));
    CHECK_ALLOC_ERR(t->rows_v)
}


/* in an existing selection, it finds the cell with the min/max numeric value and sets the selection to it */
void dcol_f(cl_t *cl, tab_t *t)
{
#define c_from cl->cmds[cl->currsel].col_1
#define c_to cl->cmds[cl->currsel].col_2

    /* Tn the first option, a whole row is about to be deleted */
    if(c_from == 1 && c_to >= t->col_c + 1)
    {
        delete_table(t);
    }

        /* Remove not all columns */
    else
    {
        int diff = 0;

        /* walk through all rows from the end */
        for(int r = t->len - 1; r >= 0; r--)
        {
            /* firstly free all allocated cells */
            for(int c = c_from - 1; c < c_to; c++)
            {
                free(t->rows_v[r].cols_v[c].elems_v);
            }
            diff = c_to - c_from + 1;

            /* shift all columns */
            for(int c = c_from - 1; c + diff < t->rows_v[r].len; c++)
            {
                t->rows_v[r].cols_v[c] = t->rows_v[r].cols_v[c + diff];
            }

            t->rows_v[r].cols_c -= diff; /* change number of columns inn the table */
            t->rows_v[r].len -= diff; /* --//-- */
        }
        t->col_c -= diff; /* change number of columns in the table */
    }
}

#undef c_from
#undef c_to
//endregion


//region PROCESSING
/**
 * Call functions for processing the selection
 */
void process_sel(cl_t *cl, tab_t *t, int *exit_code)
{

    /* find first cell in the current selection contains the pattern and set cell selection on it */
    if(cl->cmds[cl->cmds_c].proc_opt == FIND)
    {find_f(cl, t, exit_code);}

        /* define temp selection */
    else if(cl->cmds[cl->cmds_c].proc_opt == DEF)
    {def_f(cl);}

        /* change cell selection to the temp selection */
    else if(cl->cmds[cl->cmds_c].proc_opt == USE)
    {use_f(cl, t, exit_code);}

        /* increase numberical value in the temp selection */
    else if(cl->cmds[cl->cmds_c].proc_opt == INC)
    {inc_f(cl, t, exit_code);}

        /* set column in range of the selection by numerical value */
    else if(cl->cmds[cl->cmds_c].proc_opt == MIN || cl->cmds[cl->cmds_c].proc_opt == MAX)
    {min_max_f(cl, t, exit_code, cl->cmds[cl->cmds_c].proc_opt);}

        /* set current selection to the buffsel which is a buffer selection ."temp_sel" has been already occupied */
    else if(cl->cmds[cl->cmds_c].proc_opt == SETTMP)
    {set_tmp_f(cl);}

        /* set buffered selection _ to the current selection */
    else if(cl->cmds[cl->cmds_c].proc_opt == USETMP)
    {use_tmp_f(cl);}
}

/**
 *  Call functions for processing the table
 */
void process_table(cl_t *cl, tab_t *t, int *exit_code)
{
    if(cl->cmds[cl->cmds_c].proc_opt != DCOL && cl->cmds[cl->cmds_c].proc_opt != DROW)
    {CHECK_TAB_DELETED}
    /* determine the length of the cell from one cell and write it in to another cell */
    if(cl->cmds[cl->cmds_c].proc_opt == LEN)
    {len_f(cl, t, exit_code);}

        /* coout non empty columns in the current selection */
    else if(cl->cmds[cl->cmds_c].proc_opt == COUNT)
    {count_f(cl, t, exit_code);}

        /* arithmetical functions */
    else if(cl->cmds[cl->cmds_c].proc_opt >= AVG && cl->cmds[cl->cmds_c].proc_opt <= SUM)
    {avg_sum_f(cl, t, exit_code);}

        /* swap columns */
    else if(cl->cmds[cl->cmds_c].proc_opt == SWAP)
    {swap_f(cl, t);}

        /* set a strig to the cell */
    else if(cl->cmds[cl->cmds_c].proc_opt == SET)
    {set_f(t, cl, exit_code);}

        /* append/insert a row */
    else if(cl->cmds[cl->cmds_c].proc_opt == AROW || cl->cmds[cl->cmds_c].proc_opt == IROW)
    {irow_arow_f(cl, t, exit_code, cl->cmds[cl->cmds_c].proc_opt);}

        /* delete row */
    else if(cl->cmds[cl->cmds_c].proc_opt == DROW)
    {drow_f(cl, t, exit_code);}

        /* insert/append a column */
    else if(cl->cmds[cl->cmds_c].proc_opt == ICOL || cl->cmds[cl->cmds_c].proc_opt == ACOL)
    {icol_acol_f(cl, t, exit_code, cl->cmds[cl->cmds_c].proc_opt);}

        /* celar the table */
    else if(cl->cmds[cl->cmds_c].proc_opt == CLEAR)
    {
        clear_f(cl->cmds[cl->currsel].row_1, cl->cmds[cl->currsel].col_1, cl->cmds[cl->currsel].row_2,
                cl->cmds[cl->currsel].col_2, t);
    }

        /* delete columns */
    else if(cl->cmds[cl->cmds_c].proc_opt == DCOL)
    {dcol_f(cl, t);}

        /* actually this statement must not be reached */
    else
    {*exit_code = UNREC_CMD_ERR;}

}
//endregion


//region cmdLINE ARGS PARSING

void add_ptrn(carr_t *cmd, char *arr, char *ptrn, int *exit_code)
{
    if(strlen(cmd->elems_v + strlen(ptrn)) < PTRNLEN)
    {
        /* write a pattern to the structure */
        memcpy(arr, cmd->elems_v + strlen(ptrn), strlen(cmd->elems_v + strlen(ptrn)));
    }
        /* pattern is too long */
    else
    {
        *exit_code = LEN_UNSUPCMD_ERR;
        CHECK_EXIT_CODE
    }
}


void extract_nums(cl_t *cl, char n_extr_nums[PTRNLEN], int *exit_code)
{
    char *token = NULL;
    char *ptr = NULL;
    int nums[5] = {0}; /* to return an error code if smething like [a,a,a,a,a,a,a] will be entered */
    int i = 0;

    /* get the first token */
    token = strtok(n_extr_nums, ",");

    /* walk through other tokens */
    for(; i < 2 && token != NULL; i++)
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
        } else if(i == 1)
        {
            cl->cmds[cl->cmds_c].col_1 = (cl->cmds[cl->cmds_c].col_2 = nums[i]);
        } else
        {
            *exit_code = VAL_UNSUPARG_ERR;
        }
        token = strtok(NULL, ",");
    }
    CHECK_EXIT_CODE
}


/**
 *
 * @param cl
 * @param n_extr_nums
 * @param exit_code
 */
void tmp_cmds_extract_nums(cl_t *cl, char n_extr_nums[PTRNLEN], int *exit_code)
{
    char *junk = NULL;
    int num = 0;
    num = (int)strtol(n_extr_nums, &junk, 10);

    /* if there is a wrong number in the command or number is too large */
    if(junk[0] || num > NUM_TMP_SELS - 1 || num < 0)
    {
        *exit_code = VAL_UNSUPARG_ERR;
        CHECK_EXIT_CODE
    }

    /* partly initialize the command */
    cl->cmds[cl->cmds_c].row_1 = num; /* represents the number of temp_sel command */
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

    /** SELECTION **/
    /* functinos for working with temporary variables */
    if(!strncmp("def _", cmd->elems_v, strlen("def _")))
    {
        cl->cmds[cl->cmds_c].proc_opt = DEF;
        add_ptrn(cmd, n_extr_nums, "def _", exit_code);
    } else if(!strncmp("use _", cmd->elems_v, strlen("use _")))
    {
        cl->cmds[cl->cmds_c].proc_opt = USE;
        add_ptrn(cmd, n_extr_nums, "use _", exit_code);
    } else if(!strncmp("inc _", cmd->elems_v, strlen("inc _")))
    {
        cl->cmds[cl->cmds_c].proc_opt = INC;
        add_ptrn(cmd, n_extr_nums, "inc _", exit_code);
    }
        /* The string STR can be enclosed in quotation marks and can contain special characters preceded by a slash */
    else if(!strncmp("find ", cmd->elems_v, strlen("find ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = FIND;
        add_ptrn(cmd, cl->cmds[cl->cmds_c].pttrn, "find ", exit_code);
        CHECK_EXIT_CODE
    }

        /** PROCESSING **/
        /* The string STR can be enclosed in quotation marks and can contain special characters preceded by a slash */
    else if(!strncmp("set ", cmd->elems_v, strlen("set ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = SET;
        add_ptrn(cmd, cl->cmds[cl->cmds_c].pttrn, "set ", exit_code);
    }
        /* commands use a cell as an argument */
    else if(!strncmp("swap ", cmd->elems_v, strlen("swap ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = SWAP;
        add_ptrn(cmd, n_extr_nums, "swap ", exit_code);
    } else if(!strncmp("sum ", cmd->elems_v, strlen("sum ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = SUM;
        add_ptrn(cmd, n_extr_nums, "sum ", exit_code);
    } else if(!strncmp("avg ", cmd->elems_v, strlen("avg ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = AVG;
        add_ptrn(cmd, n_extr_nums, "avg ", exit_code);
    } else if(!strncmp("count ", cmd->elems_v, strlen("count ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = COUNT;
        add_ptrn(cmd, n_extr_nums, "count ", exit_code);
    } else if(!strncmp("len ", cmd->elems_v, strlen("len ")))
    {
        cl->cmds[cl->cmds_c].proc_opt = LEN;
        add_ptrn(cmd, n_extr_nums, "len ", exit_code);
    }
    CHECK_EXIT_CODE


    if(cl->cmds[cl->cmds_c].proc_opt >= SET && cl->cmds[cl->cmds_c].proc_opt <= LEN)
    {
        cl->cmds[cl->cmds_c].cmd_opt = PRC;

        /* command set have no numbers to extract, other have */
        if(cl->cmds[cl->cmds_c].proc_opt != SET)
        {
            extract_nums(cl, n_extr_nums, exit_code);
        }
    } else if(cl->cmds[cl->cmds_c].proc_opt >= DEF && cl->cmds[cl->cmds_c].proc_opt <= FIND)
    {
        cl->cmds[cl->cmds_c].cmd_opt = SEL;
        /* extract numbers from commands working with temporary variables */
        if(cl->cmds[cl->cmds_c].proc_opt >= DEF && cl->cmds[cl->cmds_c].proc_opt <= INC)
        {
            tmp_cmds_extract_nums(cl, n_extr_nums, exit_code);
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
    {cl->cmds[cl->cmds_c].proc_opt = USETMP;}

        /* if there's a selection cmd sets a new current selection */
    else
    {
        get_nums(cmd, cl, t, exit_code);
        cl->cmds[cl->cmds_c].cmd_opt = SEL;
        cl->cmds[cl->cmds_c].proc_opt = CHANGESEL;
    }
    if(cl->cmds[cl->cmds_c].proc_opt >= MIN && cl->cmds[cl->cmds_c].proc_opt <= USETMP)
    {cl->cmds[cl->cmds_c].cmd_opt = SEL;}

    else if(cl->cmds[cl->cmds_c].proc_opt >= SET && cl->cmds[cl->cmds_c].proc_opt <= DCOL)
    {cl->cmds[cl->cmds_c].cmd_opt = PRC;}

    free(currcom);
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
 * exit_code */
void process_cmd(cl_t *cl, tab_t *t, int *exit_code)
{
    /* expand tab to fit the selection */
    expand_tab(t, cl->cmds[cl->cmds_c].row_2, cl->cmds[cl->cmds_c].col_2, exit_code);
    CHECK_EXIT_CODE

    switch(cl->cmds[cl->cmds_c].cmd_opt)
    {
        case SEL: /* selection is already set in the functinon */
            process_sel(cl, t, exit_code);
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
    cmd->isinit = false;
    cmd->cmd_opt = NOPT;
    memset(cmd->pttrn, 0, PTRNLEN);
}


void init_cmd(carr_t *cmd, tab_t *t, cl_t *cl, int *exit_code)
{
    /* fill everyrhing with 0s */
    create_cmd(&cl->cmds[cl->cmds_c]);

    cell_trim(cmd, exit_code); /* trim a command */ //
    CHECK_EXIT_CODE

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

    //region variables
    int arglen = (int)strlen(arg);
    bool quoted = false;
    bool backslashed = false;

    /* firsst cell selection command is on the position 0,
     * also it is neccesarry to init cmds count */
    cl->cellsel = (cl->cmds_c = 0);

    /* set a selection to the first cell(by default) */
    set_sel(cl, 1, 1, 1, 1, exit_code, RCCELL);

    /* current command(0) is a selection */
    cl->cmds[cl->cmds_c].cmd_opt = SEL;

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

            /* process an initialized cmd */
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
        } else if(arg[p] == '\"')
        {
            if(backslashed)
            {
                a_carr(cmd, '\\', exit_code);
                a_carr(cmd, arg[p], exit_code);
                NEG(backslashed)
            } else
            {NEG(quoted)}
        } else if((arg[p] == '[' || arg[p] == ']'))
        {
            if(backslashed)
            {
                a_carr(cmd, arg[p], exit_code);
                NEG(backslashed)
            }
            if(quoted)
            {
                a_carr(cmd, arg[p], exit_code);
            }
        } else
        {
            if(backslashed)
            {NEG(backslashed)}
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
        *exit_code = W_SEPARATORS_ERR;
        CHECK_EXIT_CODE
    }
}
//endregion

/**
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
        return;
    }

    /* copy an argument to the array and call the function for it */
    init_cmds(&cmd, argv[clarg], cl, t, exit_code);
    free(cmd.elems_v);
    CHECK_EXIT_CODE
}

void print_error_message(const int *exit_code)
{
    /* an array with all error messages */
    char *error_msg[] =
            {
                    "Cannot allocate/reallocate memoory",
                    "Entered separators are not supported by the program",
                    "You've entered wrong arguments",
                    "Cmd you've entered has unsupported value",
                    "Unrecognized argument in the commandline",
                    "There is no file with this name",
                    "Unsupported length of the command",
                    "Unrecognized command",
                    "Working with undefined temporary command",
                    "Bad input file"
            };

    /* print an error message to stderr */
    fprintf(stderr, "Error %d: %s.\n", *exit_code, error_msg[*exit_code - 1]);
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

    /* allocate a new empty table */
    table_ctor(&t, 1, 1, 1, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* initialize a file pointer */
    cl.ptr = NULL;

    /* initialize separators from cmdline */
    init_separators(argc, argv, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* adds table to the structure */
    get_table(argc, argv, &t, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* parse cmdline arguments and process table for them */
    parse_cl_proc_tab(&argc, argv, &t, &cl, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* beautify the table */
    prepare_tab_bef_printing(&cl.seps, &t, &exit_code);


    print_tab(&t, &cl.seps, cl.ptr);

    clear_data(&t, &cl);

    /* exit_code is -1 means table has been processed successfully */
    return (exit_code == -1) ? 0 : exit_code;
}

/* Prints documentation, Author */
void print_documentation()
{
    printf(" Project: 2 - simple spreadsheet editor 2\n"
           " Subject: IZP 2020/21\n"
           " Author: Skuratovich Aliaksandr xskura01@fit.vutbr.cz\n");
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
