#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>		// Para usar strings

#ifdef WIN32
#include <windows.h>    // Apenas para Windows
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>     // Funções da OpenGL
#include <GL/glu.h>    // Funções da GLU
#include <GL/glut.h>   // Funções da FreeGLUT
#endif

// SOIL é a biblioteca para leitura das imagens
#include "SOIL.h"

// Um pixel RGB (24 bits)
typedef struct {
    unsigned char r, g, b;
} RGB;

// Uma imagem RGB
typedef struct {
    int width, height;
    RGB* img;
} Img;

// Protótipos
void load(char* name, Img* pic);
void uploadTexture();

// Funções da interface gráfica e OpenGL
void init();
void draw();
void keyboard(unsigned char key, int x, int y);

// Largura e altura da janela
int width, height;

// Identificadores de textura
GLuint tex[3];

// As 3 imagens
Img pic[3];

// Imagem selecionada (0,1,2)
int sel;

// Carrega uma imagem para a struct Img
void load(char* name, Img* pic)
{
    int chan;
    pic->img = (RGB*) SOIL_load_image(name, &pic->width, &pic->height, &chan, SOIL_LOAD_RGB);
    if(!pic->img)
    {
        printf( "SOIL loading error: '%s'\n", SOIL_last_result() );
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, chan);
}

int main(int argc, char** argv)
{
    if(argc < 2) {
        printf("seamcarving [origem] [mascara]\n");
        printf("Origem é a imagem original, mascara é a máscara desejada\n");
        exit(1);
    }
	glutInit(&argc,argv);

	// Define do modo de operacao da GLUT
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

	// pic[0] -> imagem original
	// pic[1] -> máscara desejada
	// pic[2] -> resultado do algoritmo

	// Carrega as duas imagens
    load(argv[1], &pic[0]);
    load(argv[2], &pic[1]);

    if(pic[0].width != pic[1].width || pic[0].height != pic[1].height) {
        printf("Imagem e máscara com dimensões diferentes!\n");
        exit(1);
    }

    // A largura e altura da janela são calculadas de acordo com a maior
    // dimensão de cada imagem
    width = pic[0].width;
    height = pic[0].height;

    // A largura e altura da imagem de saída são iguais às da imagem original (1)
    pic[2].width  = pic[1].width;
    pic[2].height = pic[1].height;

	// Especifica o tamanho inicial em pixels da janela GLUT
	glutInitWindowSize(width, height);

	// Cria a janela passando como argumento o titulo da mesma
	glutCreateWindow("Seam Carving");

	// Registra a funcao callback de redesenho da janela de visualizacao
	glutDisplayFunc(draw);

	// Registra a funcao callback para tratamento das teclas ASCII
	glutKeyboardFunc (keyboard);

	// Cria texturas em memória a partir dos pixels das imagens
    tex[0] = SOIL_create_OGL_texture((unsigned char*) pic[0].img, pic[0].width, pic[0].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
    tex[1] = SOIL_create_OGL_texture((unsigned char*) pic[1].img, pic[1].width, pic[1].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

    // Exibe as dimensões na tela, para conferência
    printf("Origem  : %s %d x %d\n", argv[1], pic[0].width, pic[0].height);
    printf("Destino : %s %d x %d\n", argv[2], pic[1].width, pic[0].height);
    sel = 0; // pic1

	// Define a janela de visualizacao 2D
	glMatrixMode(GL_PROJECTION);
	gluOrtho2D(0.0,width,height,0.0);
	glMatrixMode(GL_MODELVIEW);

    // Aloca memória para a imagem de saída
	pic[2].img = malloc(pic[1].width * pic[1].height * 3); // W x H x 3 bytes (RGB)
	// Pinta a imagem resultante de preto!
	memset(pic[2].img, 0, width*height*3);

    // Cria textura para a imagem de saída
	tex[2] = SOIL_create_OGL_texture((unsigned char*) pic[2].img, pic[2].width, pic[2].height, SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	// Entra no loop de eventos, não retorna
    glutMainLoop();
}


// Gerencia eventos de teclado
void keyboard(unsigned char key, int x, int y)
{
    if(key==27) {
      // ESC: libera memória e finaliza
      free(pic[0].img);
      free(pic[1].img);
      free(pic[2].img);
      exit(1);
    }
    if(key >= '1' && key <= '3')
        // 1-3: seleciona a imagem correspondente (origem, máscara e resultado)
        sel = key - '1';
    if(key == 's') {
        // Aplica o algoritmo e gera a saida em pic[2].img...
        // ...
        // ... (crie uma função para isso!)

        // Exemplo: pintando tudo de amarelo
        //for(int i=0; i<pic[2].height*pic[2].width; i++)
        //    pic[2].img[i].r = pic[2].img[i].g = 255;
        calculaEnergia();

        // Chame uploadTexture a cada vez que mudar
        // a imagem (pic[2])
        uploadTexture();
    }
    glutPostRedisplay();
}


void matrizAcumulada(int m[pic[0].height*pic[0].width]){
    int pixSupEsq;
    int pixSupDir;
    int pixSup;
    int i;
    int menorPix = 0;

    for(i=pic[0].width + 1; i<pic[0].height*pic[0].width; i++){

        if (!verificaMascara(i,&m[i])){


            pixSup = m[i - pic[0].width];
            menorPix = pixSup;

            if ( (i - pic[0].width) +1 <= pic[0].width){
                pixSupDir = m[(i - pic[0].width) +1];
                if (menorPix > pixSupDir) menorPix =pixSupDir;
            }



            if ( (i - pic[0].width) -1 <= 0){
                pixSupEsq = m[(i - pic[0].width)-1];
                if (menorPix > pixSupEsq) menorPix = pixSupEsq;
            }

            m[i] = menorPix + m[i];

        }else{
            //printf("  %d %s\n",m[i]," ASDQWE ");
        }


    }
    printf("  %d %s\n",m[0]," ASDQWE ");

}

//int refMatriz (int * m){
 //   for(i=0; i<pic[0].height*pic[0].width; i++){

  //  }
//}


int calculaEnergia(){
    int i=0;
    int matriz [pic[0].height*pic[0].width];
    int esq,dir,cima,baixo;
    int custo = 0 ;
        for(i=0; i<pic[0].height*pic[0].width; i++){
            pic[0].img[i] ; // pixel a ser calculado


            if (i - pic[0].width  >= 0){ //pixel acima de i
              cima= (i - pic[0].width)  ; //pixel acima de i
            }else{
              cima= ( ((pic[0].height * pic[0].width) - 1) - i); //pixel acima de i
            }


            if (i + pic[0].width <= pic[0].height){ //pixel abaixo de i
               baixo = i + pic[0].width; //pixel abaixo de i
            }else {
               baixo=  (abs(pic[0].width -(pic[0].height * pic[0].width - i)));
            }


            if (i - 1 >= 0){ //pixel a esquerda de i
                esq = i - 1 ; //pixel a esquerda de i
            }else {
                esq = i + (pic[0].height - 1);
            }

            if (i + 1 <= pic[0].width){ //pixel a direita de i
               dir =  i + 1; //pixel a direita de i
            }else{
               dir =  i + (pic[0].height - 1);
            }

           // custo = formulaEnergia(pic[0].img[196095],pic[0].img[511],pic[0].img[196606],pic[0].img[196990]);
            custo = formulaEnergia(pic[0].img[cima],pic[0].img[baixo],pic[0].img[esq],pic[0].img[dir]);

            matriz[i] = custo;

            //printf("\n %d %S",matriz[i]," a ");
            //printf("%d %d %d %d",cima,baixo,esq, dir);

        }
        printf("%d\n",custo);
        printf(" %d %d %d %d",cima,baixo,esq, dir);
        matrizAcumulada(matriz);
        return matriz;
}

int verificaMascara(int i,int * peso){

    if ( pic[1].img[i].b == 0) {
        if (pic[1].img[i].r > 0 && pic[1].img[i].g == 0 ){
                *peso  = INT_MIN;
                return 1;
        }else {
            if (pic[1].img[i].g > 0 && pic[1].img[i].r == 0){
                *peso  = INT_MAX;
                return 1;
            }
        }
    }
    return 0;

}


int formulaEnergia (RGB cima,RGB baixo,RGB esq,RGB dir){
    int deltaX, deltaY,energia;
    deltaX = pow((dir.r - esq.r), 2) + pow((dir.b - esq.b), 2) + pow((dir.g - esq.g), 2);
    deltaY = pow((cima.r - baixo.r),2) + pow((cima.b - baixo.b),2) + pow ((cima.g - baixo.g),2);
    energia = deltaX + deltaY;
    return energia;
}

// Faz upload da imagem para a textura,
// de forma a exibi-la na tela
void uploadTexture()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        pic[2].width, pic[2].height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, pic[2].img);
    glDisable(GL_TEXTURE_2D);
}

// Callback de redesenho da tela
void draw()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Preto
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // Para outras cores, veja exemplos em /etc/X11/rgb.txt

    glColor3ub(255, 255, 255);  // branco

    // Ativa a textura corresponde à imagem desejada
    glBindTexture(GL_TEXTURE_2D, tex[sel]);
    // E desenha um retângulo que ocupa toda a tela
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0,0);
    glVertex2f(0,0);

    glTexCoord2f(1,0);
    glVertex2f(pic[sel].width,0);

    glTexCoord2f(1,1);
    glVertex2f(pic[sel].width, pic[sel].height);

    glTexCoord2f(0,1);
    glVertex2f(0,pic[sel].height);

    glEnd();
    glDisable(GL_TEXTURE_2D);

    // Exibe a imagem
    glutSwapBuffers();
}
