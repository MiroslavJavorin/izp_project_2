/**
 * Project 2 - simple spreadsheet editor 2
 * Subject IZP 2020/21
 * @Author Skuratovich Aliaksandr xskura01@fit.vutbr.cz
*/

// TODO в случае проблемы расширь таблицу

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
 *  SHOWTAB     - shows operations
 *  MEMBUG   - debug memory usage
 *  SEPSBUG  - debug separators
 *  TABUG    - debug for see the table
 * */

/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */
#define CHECK_EXIT_CODE_IN_RUN_PROGRAM if(exit_code > 0)\
{\
    print_error_message(&exit_code);\
    return exit_code;\
}

#define CHECK_EXIT_CODE if(*exit_code > 0){ return; error_line_global = __LINE__; }

#define CHECK_ALLOC_ERR(arr) if(arr == NULL)\
{\
    *exit_code = W_ALLOCATING_ERROR;\
    error_line_global = __LINE__;\
    return;\
}

#define BUFF_S 1

#define NEG(n) ( n = !n );

#define FREE(arr) if(arr != NULL) { free(arr); }

//endregion

//region enums
enum erorrs
{
    W_ALLOCATING_ERROR = 1,
    W_SEPARATORS_ERROR,
    NUM_ARGUNSUP_ERROR,
    VAL_ARGUNSUP_ERROR,
    NO_SUCH_FILE_ERROR,
    Q_DONT_MATCH_ERROR
};

//endregion

//region structures
/* contains information about delim string */
typedef struct carr_t
{
    int  elems_c; /* number of elements array contains */
    char *elems_v;   /* dynamically allocated array of chars  */
    int  length;   /* size of an array */
    bool isempty;
} carr_t;

/* contains information about arguments user entered in commandline */
typedef struct
{
    carr_t seps;
    FILE  *ptr;
} clargs_t;

/* contains information about a row */
typedef struct row_t
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

void print_tab(tab_t *t, carr_t *seps)
{
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

void carr_ctor(carr_t *arr, int *exit_code)
{
    arr->elems_v = (char*)calloc(BUFF_S, sizeof(char)); // FIXME if someting foesnt work change to malloc
    CHECK_ALLOC_ERR(arr->elems_v)

    arr->length = BUFF_S;
    arr->elems_c = 0;
    arr->isempty = true;
}

/* allocating a row of size BUFF_S */
void row_ctor(row_t *r, int *exit_code)
{
    r->cols_v = (carr_t*)malloc( BUFF_S * sizeof(carr_t));
    CHECK_ALLOC_ERR(r->cols_v)

    /* allocate cols */
    for(int cols = 0; cols < BUFF_S; cols++)
    {
        carr_ctor(&r->cols_v[cols], exit_code);
        CHECK_EXIT_CODE
    }
    r->length = BUFF_S;
    r->cols_c = 0;
}

void table_ctor(tab_t *t, int* exit_code)
{
    /* allocate thw new table */
    t->rows_v = (row_t*)malloc(BUFF_S * sizeof(row_t));
    CHECK_ALLOC_ERR(t->rows_v)

    /* allocate rows */
    for(int row = 0; row < BUFF_S; row++)
    {
        row_ctor(&t->rows_v[row], exit_code);
        CHECK_EXIT_CODE
    }
    t->row_c = 0;
    t->length = BUFF_S;
}

/**
 *  Adds a character to an array.
 *  If there is no more memory in the array allocate new memory
 *
 * @param arr        dynamically allocated array.
 * @param item       an item to add to an array
 * @param exit_code  can be changed if array reallocated unsuccesfully
 */
void a_carr(carr_t *arr, char *item, int *exit_code)
{
    /* if there is no more space for the new element add new space */
    if(arr->elems_c + 1 >= arr->length )
    {
        arr->length += BUFF_S;
        arr->elems_v = (char*)realloc(arr->elems_v, arr->length  * sizeof(char));
        CHECK_ALLOC_ERR(arr->elems_v)
    }
    arr->isempty = false;
    arr->elems_v[arr->elems_c++] = *item;
}

/* returns true if given set contains the item*/
bool set_contains(carr_t *set, char *item)
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
void set_add_item(carr_t *set, char *item, int *exit_code)
{
    if(!set_contains(set, item))
    {
        a_carr(set, item, exit_code);
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
void init_separators(const int argc, const char **argv, clargs_t *clargs, int *exit_code)
{
    //region variables
    clargs->seps.length = 0;
    carr_ctor(&clargs->seps, exit_code);
    CHECK_EXIT_CODE
    int k = 0;
    char charchar;
    //endregion

    /* if there is only name, functions and filename */
    if(argc >= 3 && strcmp(argv[1], "-d"))
    {
        charchar = ' ';
        a_carr(&clargs->seps, &charchar, exit_code);
        CHECK_EXIT_CODE
    }

    /* if user entered -d flag and there are function names and filename in the commandline */
    else if(argc >= 5 && !strcmp(argv[1], "-d"))
    {
        while(argv[2][k])
        {
            /* checks if the user has not entered characters that will lead to undefined program behavior */
            if(argv[2][k] == 10 || argv[2][k] == 92 || argv[2][k] == 34)
            {
                *exit_code = W_SEPARATORS_ERROR;
                return;
            }
            charchar = argv[2][k];
            set_add_item(&(clargs->seps), &charchar, exit_code);
            CHECK_EXIT_CODE
            k++;
        }
    }

    /* If number of arguments is less than or equal to 2 which means t
     * here are only name of the program and filename (optional) */
    else if(argc <= 4)
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

//region Trim

/* trims an array of characters */
void cell_trim(carr_t *arr, int *exit_code)
{
    if(!arr->elems_v)
    {
        printf("\t\t\t э кумыс\n");
        arr->elems_v = (char*)calloc(BUFF_S ,sizeof(char));
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
        a_carr(col, &buff_c, exit_code);

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
            row->length += BUFF_S;
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

void get_table(const int argc, const char **argv, tab_t *t, clargs_t *clargs, int *exit_code)
{
    //TODO check if file is not empty
    if((clargs->ptr = fopen(argv[argc - 1], "r+")) == NULL)
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
                t->length += BUFF_S;
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

void process_table(clargs_t *clargs, tab_t *t, int *exit_code)
{
    (void) clargs;
    (void) exit_code;
    (void) t;
}

void print_error_message(int *exit_code)
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
    init_separators(argc, argv, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* adds table to the structure */
    get_table(argc, argv, &t, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    print_tab(&t, &clargs.seps);

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
