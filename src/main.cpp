
void run_tests();

int compile(const char *);

int main(int argc, char **argv)
{
    if (argc < 2)
        run_tests();
    else
        return compile(argv[1]);

    return 0;
}
