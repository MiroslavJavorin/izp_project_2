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

#define NOF_TMP_SELS 10 /* max number of temo selections */

#define NEG(n) ( n = !n ); /* negation of bool value */

#define FREE(arr) if(arr) { free(arr); } /* check if a is not NULL */

//endregion

//region enums

enum erorrs
{
    W_ALLOCATING_ERROR = 1, /* error caused by a 'memory' function */
    W_SEPARATORS_ERROR,     /* wrong separators have been entered*/
    NUM_ARGUNSUP_ERROR,     /* unsupported number of arguments */
    VAL_ARGUNSUP_ERROR,     /* wrong arguments in the commandline */
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

enum changesel
{
    ROW1COL1 = 1,
    ROW2COL2 = 3
};

enum tedit
{
    IROW,
    AROW,
    DROW,
    ICOL,
    ACOL,
    DCOL
};

typedef enum
{
    DAPROCOPT,
    TAEDITOPT,
    CHGSELOPT,
    TMPSELOPT,
    SEQENCOPT 
} argopt;
//endregion

//region structures

/* constains information about column selection */
typedef struct 
{
    /* upper row, left column, lower row, right column */
    int row_u, col_l, row_l, col_r;
    bool isempty;
} colsel_t;

/* contains information about delim string */
typedef struct carr_t
{
    int  elems_c;  /* number of elements array contains */
    char *elems_v; /* dynamically allocated array of chars  */
    int  length;   /* size of an array */
    bool isempty;
} carr_t;

/* contains information about arguments user entered in commandline */
typedef struct
{
    carr_t seps;
    bool defaultsep; /* means ' ' is used as a searator */
    FILE  *ptr;

    /* column selections */
    colsel_t currsel;               /* current selection */
    colsel_t tempsel[NOF_TMP_SELS]; /* array with temporary commands */
    
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

    arr->length = MEMBLOCK;
    arr->elems_c = 0;
    arr->isempty = true;
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

/* returns true if given set contains the item*/
bool set_contains(carr_t *set, const char *item)
{
    for(int i = 0; i < set->elems_c; i++)
        if(set->elems_v[i] == *item)
             return true;

     return false;
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
    if(!set_contains(set, &item))
    {
        a_carr(set, item, exit_code);
        CHECK_EXIT_CODE
    }
}

/**
 * Returns the number of occurancef of the given char in the given array
 *
 * @param arr
 * @param charchar
 * @return
 */
int numoc(carr_t *carr, char charchar)
{
    int n = 0;
    int i = 0;
    while(i < carr->elems_c)
    {
        if(charchar == carr->elems_v[i])
            n++;
        i++;
    }
    return n;
}


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

//region GET FILE
/* returns true is newline or eof reached */
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
        else if(set_contains(&clargs->seps, &buff_c) && !quoted)
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

void process_command(carr_t *command, clargs_t *clargs, tab_t *t, int *exit_code)
{
    /* number of occurances */
    int changeselopt = numoc(command, ',');
    char *currcom = calloc(command->elems_c, sizeof(char));
    CHECK_ALLOC_ERR(currcom)
    
    /* means there's a Rows and columns selection in the command or (TODO)STR*/
    if(changeselopt == ROW1COL1) 
    {
        sscanf(command->elems_v, "[%d,%d]", &clargs->currsel.row_u, &clargs->currsel.col_l);
        sprintf(currcom, "[%d,%d]", clargs->currsel.row_u, clargs->currsel.col_l );

        if(strcmp(currcom, command->elems_v) != 0)
        {
            printf("пошел нахуй чмо\n"); // TODO here can be a STR
            error_line_global = __LINE__;
            *exit_code = VAL_ARGUNSUP_ERROR;
            return;
        }
        printf(" %d %d %d %d\n", clargs->currsel.row_u, clargs->currsel.col_l,
               clargs->currsel.row_l, clargs->currsel.col_r);
    }

    /* if there is a wondow of cells */
    else if(changeselopt == ROW2COL2)
    {
        sscanf(command->elems_v, "[%d,%d,%d,%d]",
                &clargs->currsel.row_u, &clargs->currsel.col_l,
                &clargs->currsel.row_l, &clargs->currsel.col_r
                );
        sprintf(currcom, "[%d,%d,%d,%d]",
                clargs->currsel.row_u, clargs->currsel.col_l,
                clargs->currsel.row_l, clargs->currsel.col_r);

        if(strcmp(currcom, command->elems_v) != 0)
        {
            printf("пошел нахуй чмо\n"); // TODO here can be a STR
            error_line_global = __LINE__;
            *exit_code = VAL_ARGUNSUP_ERROR;
            return;
        }
        printf(" %d %d %d %d\n", clargs->currsel.row_u, clargs->currsel.col_l,
               clargs->currsel.row_l, clargs->currsel.col_r);
    }

    /* if there's another command */
    else
    {
        // TODO [min] [max] , set STR , clear, swap [R,C], sum [R,C] avg [R,C], count [R,C], len [R,C]
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
void parse_arg(carr_t *dst, const char *src, clargs_t *clargs, tab_t *t, int *exit_code)
{
    int arglen = (int)strlen(src);

    /* write argument to an array */
    for(int p = 0; p < arglen && src[p] != ';'; p++)
    {
        a_carr(dst, src[p], exit_code);
        CHECK_EXIT_CODE
    }
    cell_trim(dst, exit_code);
    CHECK_EXIT_CODE

    process_command(dst, clargs, t, exit_code);
    CHECK_EXIT_CODE

    printf("%s\n", dst->elems_v);
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
        *exit_code = NUM_ARGUNSUP_ERROR;
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
    /*
     * TODO
        2 - for each element parse an array and
            1 - change the selsction
            2 - call a function
    */

    if(clarg == *argc - 1)
    {
        /* means there is no arguments entered in the command line */
        *exit_code = NUM_ARGUNSUP_ERROR;
        return;
    }

    carr_t barg; /* buhher commandline arg */
    carr_ctor(&barg, exit_code);
    CHECK_EXIT_CODE

    /* go through all commandline arguments withoud filename */
    for( ; clarg < *argc - 1; clarg++)
    {
        /* copies an argument to the array */
        parse_arg(&barg, argv[clarg], clargs, t, exit_code);
        CHECK_EXIT_CODE

        /* добавляешь то, что идет до ; или до конца аргумента, в строку DONE
         * TODO потом идешь ее парсить(в оптдельныю функцию!!!!)
         *   из отдельной функции, в зависимости от условий, вызываешь редакторивание таблицы
        */

    }
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
