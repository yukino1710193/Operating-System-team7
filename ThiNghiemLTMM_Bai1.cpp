#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>

int main() {
    char plain[10], cipher[10];
    int key, i, length;
    int result;
    printf("\n Enter the plain text: ");
    scanf("%s", plain);
    printf("\n Enter the key value: ");
    scanf("%d", &key);
    printf("\n \n  Plain Text: %s", plain);
    printf("\n \n  Encrypted text:");
    for (i = 0, length=strlen(plain); i < length; i++) {
        cipher[i] = plain[i] + key;
        if (isupper(plain[i]) && (cipher[i] > 'Z'))
            cipher[i] -= 26;
        if (islower(plain[i]) && (cipher[i] > 'z'))
            cipher[i] -= 26;
        printf("%c", cipher[i]);
    }

    printf("\n \n After Decryption: ");
    for (i = 0; i < length; i++) {
        plain[i] = cipher[i] - key;
        if (isupper(cipher[i]) && (plain[i] < 'A'))
            plain[i] += 26;
        if (islower(cipher[i]) && (plain[i] < 'a'))
            plain[i] += 26;
        printf("%c", plain[i]);
    }

    getch();
    return 0;
}
