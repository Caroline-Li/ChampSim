
int test_array[1000];
int main() {
        int sum = 0;
        for (int i = 0; i < 1000; i++) {
            test_array[i] = i;
        }
        for (int i = 0; i < 1000; i++) {
            sum += test_array[i];
        }
    return 0;
}