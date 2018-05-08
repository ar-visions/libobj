module supermod;

class Test : Super {
    int[] silly;
    
    Test cast(char *input) {
        return new Test();
    }

    void method2(int arg, int arg2) {
        int total_closure(int,long) = (int i, long l) {
            printf("body\n");
        };
    }

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

    void method(int arg, int arg2) {
        super.method(arg, arg2);
        Super.super_static();
        printf("value = %d\n", self.value5);
    }

    static int main(String[] args) {
        String[] args2 = new String[];
        Test b = new Test();
        String s = (String)"hi";
        int i = (int)b;
        char *str = (char *)s;
        printf("str = %s\n", str);

        String str1 = String.instance(args[0]);
        for (int i = 0; i < args.count; i++) {
            String str = String.instance(args[i]);
            printf("arg[%d] = %s\n", i, str.buffer);
        }
        Test t = new Test();
        if (Test.instance(str1)) {
            printf("test inherits\n");
        }
        t.method(1, 2);
        t.value5 = 1;
        printf("value = %d\n", t.value5);

        int testme3(long, int)[] = new int(long,int)[];

        int testme(long, int) = (long test, int arg) {
            int ii = 0;
            int tt = 5;
            printf("hey there %s\n", s.buffer);
            (int[] arg2) {
                Test test1 = new Test();
                printf("another test:%s %d %d", s.buffer, tt, test1.value5);
            };
        };
        testme(1, 2);
        testme3.push(testme);

        int testme4(long, int) = testme3.pop();
        testme4(3, 4);

        String[] ss = new String[];

        wchar_t[] test_array = new wchar_t[];
        test_array[0] = 1;

        int[String] test_pairs = new int[String];
        String s = (String)"Hi";
        test_pairs[s] = 1;
    
        String[int] test_pairs2 = new String[int];
        test_pairs2[10] = s;
        return 0;
    }
}
