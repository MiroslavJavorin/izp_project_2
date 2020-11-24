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

//region macros
/*
 *  MEMBUG   - debug memory usage
 *  SEPSBUG  - debug separators
 *  VALGRIND - catch valgrind messages
 * */

/* checks if exit code is greater than 0, which means error has been occurred,
 * calls function that prints an error on the stderr and returns an error */
#define CHECK_EXIT_CODE_IN_RUN_PROGRAM if(exit_code > 0)\
{\
    print_error_message(&exit_code);\
    return exit_code;\
}

#define CHECK_EXIT_CODE if(*exit_code > 0){ return; }
#define CHECK_ALLOC_ERR(arr) if(arr == NULL)\
{\
    *exit_code = ALLOCATING_ERROR;\
    return;\
}
#define BUFF_S 100

#define NEG(n) ( n = !n )

#define FREE(arr) if(arr != NULL) { free(arr); }
//endregion

//region enums
enum erorrs
{
    ALLOCATING_ERROR = 1,
    UNSUPPORTED_SEPARATORS_ERROR,
    TOO_FEW_ARGS_ERROR,
    NO_SUCH_FILE_ERROR
};

//endregion

//region structures
/* contains information about delim string */
typedef struct carr_t
{
    unsigned int  elems_c; /* number of elements array contains */
    char         *elems_v;   /* dynamically allocated array of chars  */
    unsigned int  size;   /* size of an array */
} carr_t;

/* array of integers  */
typedef struct iarr_t
{
    unsigned int  elems_c; /* number of elements array contains */
    unsigned int* elems_v;   /* dynamically allocated array of chars  */
    unsigned int  size;   /* size of an array */
} iarr_t;

/* contains information about arguments user entered in commandline */
typedef struct
{
    carr_t seps;
    FILE  *ptr;
} clargs_t;

/* contains information about a row */
typedef struct row_t
{
    carr_t      *cols_v; /* columns in the row */
    unsigned int cols_c; /* number of filled cols */
    unsigned int size;   /* size of allocated memory*/
} row_t;

/* contains information about a table */
typedef struct
{
    row_t*        rows_v; /* table contains rows which contain cells */
    unsigned int  row_c; /* number of rows of the table */
    unsigned int  size;  /* size of allocated memory*/
} table_t;
//endregion

/* frees all memory allocated by a table_t structure s */
void free_table(table_t *t)
{
    for(int row = 0; row < (int) t->size; row++)
    {
        for(int col = 0; col < (int) t->rows_v[row].size ;col++)
        {
            printf("%s ", t->rows_v[row].cols_v[col].elems_v);
        }
        printf("\n");
    }
}

/* frees all memory allocated by a clargs_t structure s */
void free_clargs(clargs_t *s)
{
    if(s->seps.elems_v)
        FREE(s->seps.elems_v)
    if(s->ptr)
        fclose(s->ptr);
}

void clear_data(table_t *table, clargs_t *clargs)
{
    free_table(table);
    free_clargs(clargs);
}

void carr_ctor(carr_t *arr, int *exit_code)
{
    arr->elems_v = (char*)calloc(1, sizeof(char));
    arr->size    = 1;
    arr->elems_c = 0;
    CHECK_ALLOC_ERR(arr->elems_v)
}

void a_carr(carr_t *arr, const char *item, int *exit_code)
{
    if(arr->size <= arr->elems_c + 2)
    {
        /* if no bytes are allocated */
        if(!arr->size)
        {
            carr_ctor(arr, exit_code);
            CHECK_EXIT_CODE
        }else
        {
            arr->size += BUFF_S;
            arr->elems_v = (char *)realloc(arr->elems_v, arr->size * sizeof(char));
            CHECK_ALLOC_ERR(arr->elems_v)
        }
    }
#ifdef MEMBUG
    printf("line %d elems -> %s size -> %d elems_c -> %d item -> %c\n", __LINE__, arr->elems_v,arr->size ,
            arr->elems_c, *item);
#endif
    arr->elems_v[arr->elems_c++] = *item;
}


bool set_contains(carr_t *set, char *item)
{
    for(unsigned int i = 0; i < set->elems_c; i++)
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
bool set_add_item(carr_t *set, char item, int *exit_code) // TODO make item a pointer to char
{
    if(!set_contains(set, &item))
    {
        set->elems_v = (char *)realloc(set->elems_v, ++set->elems_c * sizeof(char));
        if(set->elems_v == NULL)
        {
            *exit_code = ALLOCATING_ERROR;
            return false;
        }

        set->elems_v[set->elems_c - 1] = item;
        return true;
    }

    /* if this item is already in the set */
    else return false;
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
    carr_ctor(&clargs->seps, exit_code);
    CHECK_EXIT_CODE
    int k = 0;
    //endregion

    /* if there is only name, functions and filename */
    if(argc >= 3 && strcmp(argv[1], "-d"))
    {
        set_add_item(&(clargs->seps), ' ', exit_code);
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
                *exit_code = UNSUPPORTED_SEPARATORS_ERROR;
                return;
            }
            set_add_item(&(clargs->seps), argv[2][k], exit_code);
            CHECK_EXIT_CODE
            k++;
        }
    }

    /* If number of arguments is less than or equal to 2 which means t
     * here are only name of the program and filename (optional) */
    else if(argc <= 4)
    {
        *exit_code = TOO_FEW_ARGS_ERROR;
        return;
    }
#ifdef SEPSBUG
    printf("line %d separators -> ", __LINE__);
    for(unsigned int i = 0; i < clargs->seps.elems_c; i++ )
        printf("%c",clargs->seps.elems_v[i]);
    putchar(10);
#endif
}

void get_table(const int argc, const char **argv, table_t *table, clargs_t *clargs, int *exit_code)
{
    if((clargs->ptr = fopen(argv[argc - 1], "r+")) == NULL)
    {
        *exit_code = NO_SUCH_FILE_ERROR;
        return;
    }

    /* if file has been opened successfully */
    else
    {
        table->rows_v = NULL;
        table->size   = 0;
        bool quoted;
        char buff_ch;

        /* go through all lines of the file */
        for(table->row_c = 0; !feof(clargs->ptr); table->row_c++)
        {
            /* allocate new memory for rows in the table */
            if(table->row_c + 2 >= table->size)
            {
                table->size  += BUFF_S;
                table->rows_v = (row_t*)realloc(table->rows_v, table->size * sizeof(row_t));
            }

            quoted = false;
            /* initialize a row */
            table->rows_v[table->row_c].cols_c = 0;
            table->rows_v[table->row_c].size   = BUFF_S;
            table->rows_v[table->row_c].cols_v = (carr_t *)calloc(BUFF_S, sizeof(carr_t)); // TODO change to malloc?

            do /* pass one line from file to the structure */
            {
                /* get one char from the file */
                buff_ch = (char)fgetc(clargs->ptr);

                /* parse next row */
                if(buff_ch == '\n' || feof(clargs->ptr))  // TODO change to buff_ch == EOF
                {
                    break;
                }

                /* is the char is a separator and char is not quoted or if char is a right quote */
                else if((set_contains(&clargs->seps, &buff_ch) && !quoted))
                {
                    /* increase current cell(number of cells in the row), allocate it with 0s */
                    if(table->rows_v[table->row_c].cols_c + 2 >= table->rows_v[table->row_c].size)
                    {
                        /* allocate new columns */
                        table->rows_v[table->row_c].size += BUFF_S;
                        table->rows_v[table->row_c].cols_v = (carr_t*)realloc(table->rows_v[table->row_c].cols_v,
                                table->rows_v[table->row_c].size * sizeof(carr_t) );
                        CHECK_ALLOC_ERR(table->rows_v[table->row_c].cols_v)
                    }
                    /* increase number of columns in a row */
                    table->rows_v[table->row_c].cols_c++;
                    table->rows_v[table->row_c].cols_v[table->rows_v[table->row_c].cols_c].elems_c = 0;
                    table->rows_v[table->row_c].cols_v[table->rows_v[table->row_c].cols_c].size    = 0;
                    continue;
                }

                /* if character from file is a quotation mark which means cell will be quoted/unquoted*/
                else if(buff_ch == '\"')
                {
                    NEG(quoted);
                }

                /* add a character to the cell */
                a_carr(&table->rows_v[table->row_c].cols_v[table->rows_v[table->row_c].cols_c], &buff_ch, exit_code);
                CHECK_EXIT_CODE
            } while(1);
            /* decrease number of cells */
            table->rows_v[table->row_c].cols_v = (carr_t *)realloc(table->rows_v[table->row_c].cols_v,
                                                                   (table->rows_v[table->row_c].cols_c +1)* sizeof
                                                                   (carr_t));
            table->rows_v[table->row_c].size = table->rows_v[table->row_c].cols_c+1;
            printf("size =%d elems =%d for row %d\n", table->rows_v[table->row_c].size, table->rows_v[table->row_c]
                    .cols_c, table->row_c);
        }
    }
    table->rows_v = (row_t*)realloc(table->rows_v, (table->row_c +1)* sizeof(row_t*));
    table->size = table->row_c+1;
    // TODO crct_num_of_cols(); // correct the number of columns(find the longest row and add columns in other rows)
}

void process_table(clargs_t *clargs, table_t *table, int *exit_code)
{
    (void) clargs;
    (void) exit_code;
    (void) table;
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
            "Why?! And we light up the sky!!!"
    };

    /* print an error message to stderr */
    fprintf(stderr, "Error %d: %s.\n", *exit_code, error_msg[*exit_code - 1]);
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
    table_t  table;
    clargs_t clargs;
    //endregion

    /* initialize separators from commandline */
    init_separators(argc, argv, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    /* adds table to the structure */
    get_table(argc, argv, &table, &clargs, &exit_code);
    CHECK_EXIT_CODE_IN_RUN_PROGRAM

    clear_data(&table, &clargs);

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
