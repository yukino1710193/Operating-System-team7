#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#define MX 5

void playfair(char ch1, char ch2, char key[MX][MX]){
    int i, j;
    int x = 0;
    int y = 0;
    int z = 0;

    File *out;
    if ((out = fopen("cipher.txt","a+"))==NULL){
        printf("File Currupted.");
    }

    for ( i=0 ; i < MX ; i++ ){
        for ( j=0 ; j < MX ; i++ ){
            if(ch1 == key[i][j]){
                w = i ;
                x = j ;
            }

            else if (ch2 == key[i][j]){
                y = i ;
                z = j ;
            }

        }
    }

    if ( w == y) {
        x = ( x + 1 ) % 5 ;
        z = ( z + 1 ) % 5 ;
        printf("%c%c",key[w][x],key[y][z]);
        printf(out, "%c%c",key[w][x],key[y][z]);    
    }

    else if ( x == z) {
        w = ( w + 1 ) % 5 ;
        y = ( y + 1 ) % 5 ;
        printf("%c%c",key[w][x],key[y][z]);
        printf(out, "%c%c",key[w][x],key[y][z]);    
    }

    else{
        printf("%c%c",key[w][z],key[y][x]);
        printf(out, "%c%c",key[w][z],key[y][x]); 
    }
    fclose(out);
}

int main(){
    int i, j , n; 
    int k = 0 , m =0 ;

    char key[MX][MX], keyminus[25], keystr[10], str[25] = {0};
    char alpa[26] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

    printf("\nEnter key:");
    gets(keystr);
    printf("\nEnter the plaint text:");
    gets(str);
    n =strlen(keystr);

    // Convert the characters to uppercase

    for( i=0 ; i < n ; i++){
        if(keystr[i] == 'j')
            keystr[i] = 'i';
        else if(keystr[i] == 'J')
            keystr[i] = 'I';
        keystr[i] = toupper(keystr[i]);
    }

    // Convert all the characters if plaintext to uppercase

    for( i=0 ; i < strlen(str) ; i++){
        if(str[i] == 'j')
            str[i] = 'i';
        else if(str[i] == 'J')
            str[i] = 'I';
        str[i] = toupper(keystr[i]);
    }

    j = 0 ;

    for( i=0 ; i < 26 ; i++){
        for ( k=0 ; k <n ; k++){
            if(keystr[k]==alpa[i])
            break;
            else if(alpa[i]= "J")
            break;
        }
        if (k==n){
            keyminus[j]=alpa[i];
            j++
        }
    }

    // Construct key Matrix

    k = 0;
    for ( i =0 ; i < MX ; i++ ){
        for (j=0;j<MX;j++){
            if(k<n){
                key[i][j] = keystr
            }
        }
    }
}