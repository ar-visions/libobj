module supermod;

class Test : Super {
    private int value_intern;

    int value5 {
        get {
            return self.value_intern;
        }
        set (value) {
            self.value_intern = value;
        }
    } = 2;
    int another;

    Test cast(char *input) {
        return new Test();
    }

    void method(int arg, int arg2) {
        super.method(arg, arg2);
        Super.super_static();
        printf("value = %d\n", self.value);
    }

    static int main(String[] args) {
        String[] args2 = new String[];
        String test_arg = args2[0];
        Test b = new Test();
        Array<int> a = new Array<int>();
        String s = (String)"hi";
        int i = (int)b;
        char *str = (char *)s;
        printf("str = %s\n", str);

        a.push(b);
        String str1 = String.instance(args[0]);
        for (int i = 0; i < args.count; i++) {
            String str = String.instance(args[i]);
            printf("arg[%d] = %s\n", i, str.buffer);
        }
        Test t = new Test();
        Test failed = t.release();
        if (Test.instance(str1)) {
            printf("test inherits\n");
        }
        t.method(1, 2);
        t.value5 = 1;
        printf("value = %d\n", t.value5);

        int testme(short,int,char,short) = (long test, int arg) {
            int ii = 0;
            int t = 5;
            printf("hey there %s\n", s.buffer);
            (int[] arg2) {
                Test test1 = new Test();
                printf("another test:%s %d %d", s.buffer, t, test1.value5);
            };
        };

        String[] ss = new String[];

        const int i = 1 + (leave_alone1 leave_alone2)testing;
        wchar_t[] test_array = new wchar_t[];
        return 0;
    }
}
