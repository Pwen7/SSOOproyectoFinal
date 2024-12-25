#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include "cabeceras.h"

#define LONGITUD_COMANDO 100

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);

int ComprobarComando(char *strcomando,
                     char *orden,
                     char *argumento1,
                     char *argumento2);

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup);

int BuscaFich(EXT_ENTRADA_DIR *directorio,
              EXT_BLQ_INODOS *inodos,
              char *nombre);

void Directorio(EXT_ENTRADA_DIR *directorio,
                EXT_BLQ_INODOS *inodos);

int Renombrar(EXT_ENTRADA_DIR *directorio,
              EXT_BLQ_INODOS *inodos,
              char *nombreantiguo,
              char *nombrenuevo);

int Imprimir(EXT_ENTRADA_DIR *directorio,
             EXT_BLQ_INODOS *inodos,
             EXT_DATOS *memdatos,
             char *nombre);

int Borrar(EXT_ENTRADA_DIR *directorio,
           EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps,
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, 
           char *nombre,  
           FILE *fich);

int Copiar(EXT_ENTRADA_DIR *directorio,
           EXT_BLQ_INODOS *inodos,
           EXT_BYTE_MAPS *ext_bytemaps,
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, 
           EXT_DATOS *memdatos,
           char *nombreorigen, 
           char *nombredestino,  
           FILE *fich);

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio,
                             EXT_BLQ_INODOS *inodos,
                             FILE *fich);

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps,
                    FILE *fich);

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock,
                       FILE *fich);

void GrabarDatos(EXT_DATOS *memdatos,
                 FILE *fich);

int main(){
	char comando[LONGITUD_COMANDO];
	char orden[LONGITUD_COMANDO];
	char argumento1[LONGITUD_COMANDO];
	char argumento2[LONGITUD_COMANDO];
	
	int i,j;
	unsigned long int m;
   EXT_SIMPLE_SUPERBLOCK ext_superblock;
   EXT_BYTE_MAPS ext_bytemaps;
   EXT_BLQ_INODOS ext_blq_inodos;
   EXT_ENTRADA_DIR directorio[SIZE_BLOQUE];
   EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
   EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
   int entradadir;
   int grabardatos;
   FILE *fent;
   
   // Lectura del fichero completo de una sola vez
   
   fent = fopen("particion.bin","r+b");
   fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);
   memcpy(&ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
   memcpy(&directorio, (EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
   memcpy(&ext_bytemaps,(EXT_BYTE_MAPS *)&datosfich[1], SIZE_BLOQUE);//EXT_BLQ_INODOS
   memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
   memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
   
   // Buce de tratamiento de comandos
   for (;;){ //EXPLICAR
      do {
         printf (">> ");
         fflush(stdin);
         fgets(comando, LONGITUD_COMANDO, stdin);
      } while (ComprobarComando(comando,orden,argumento1,argumento2) !=0);
      
      if (strcmp(orden,"dir")==0) {
         Directorio(directorio,&ext_blq_inodos);
         continue;
      }
      else if (strcmp(orden,"info")==0) {
         LeeSuperBloque(&ext_superblock);
         continue;
      }
      else if (strcmp(orden,"bytemaps")==0) {
         Printbytemaps(&ext_bytemaps);
         continue;
      }
      else if (strcmp(orden,"rename")==0) {
         Renombrar(directorio, &ext_blq_inodos, argumento1, argumento2);
         continue;
      }
      else if (strcmp(orden,"imprimir")==0) {
         Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
         continue;
      }
      else if (strcmp(orden,"remove")==0) {
         Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, argumento1, fent);
         continue;
      }
      else if (strcmp(orden,"copy")==0) {
         Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, &ext_superblock, memdatos, argumento1, argumento2, fent);
         continue;
      }
      // Escritura de metadatos en comandos rename, remove, copy     
      Grabarinodosydirectorio(directorio,&ext_blq_inodos,fent);
      GrabarByteMaps(&ext_bytemaps,fent);
      GrabarSuperBloque(&ext_superblock,fent);
      
      if (grabardatos){
         GrabarDatos(memdatos,fent);
      }
      grabardatos = 0;
      //Si el comando es salir se habrán escrito todos los metadatos
      //faltan los datos y cerrar
      
      if (strcmp(orden,"salir")==0){
         GrabarDatos(memdatos,fent);
         fclose(fent);
         return 0;
      }
   }
}

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps){
   //Recorre ext_bytemaps->bmap_inodos para mostrar los inodos que estan ocupados (1) y los libres (0)
   printf("Inodos: ");
   for (int i=0; i<MAX_INODOS ; i++){
      printf("%d ", ext_bytemaps->bmap_inodos[i]);
   }

   //Recorre ext_bytemaps->bmap_bloques para mostrar los 25 primeros bloques que estan ocupados (1) o los libres (0)
   printf("\nBloques [0-25]: ");
   for(int i=0; i<25; i++){
      printf("%d ", ext_bytemaps->bmap_bloques[i]);
   }

   printf("\n");
}

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2){ //EXPLICAR
   //Separa el strcomando en tres parametros que seran utilizados luego pera saber si existe el comando y si esta bien utilizado
   int num_args = sscanf(strcomando, "%s %s %s", orden, argumento1, argumento2);

   //Comprobacion de si es un comando del grupo con 2 argumentos
   if (strcmp(orden, "copy") == 0 || strcmp(orden, "rename") == 0) {
      if (num_args ==3) {
         return 0;
      }
   } 
   
   //Comprobacion de si es un comando del grupo con 1 argumentos
   else if (strcmp(orden, "imprimir") == 0 || strcmp(orden, "remove") == 0) {
      if (num_args==2) {
         return 0;
      }
   } 

   //Comprobacion de si es un comando del grupo sin argumentos
   else if (strcmp(orden, "info") == 0 || strcmp(orden, "bytemaps") == 0 || strcmp(orden, "dir") == 0 || strcmp(orden, "salir") == 0) {
      return 0;
   }

   //Si el comando no existe o no se atiene a los parametros necesarios salta este mensaje
   printf("ERROR: Comando ilegal [bytemaps,copy,dir,info,imprimir,rename,remove,salir]\n");
   return -1;
}

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup){
   //Muestra todos los datos de el super bloque de manera ordenada

   //Tamaño bloques
   printf("Bloque %d Bytes\n", psup->s_block_size);
   //Inodos totales
   printf("inodos particion = %d\n", psup->s_inodes_count);
   //Inodos libres
   printf("indos libres = %d\n", psup->s_free_inodes_count);
   //Bolques totales
   printf("Bloques pariticion = %d\n", psup->s_blocks_count);
   //Bloques libres
   printf("Bloques libres = %d\n", psup->s_free_blocks_count);
   //Primer bloque despues de los metadatos
   printf("Primer bloque de datos = %d\n", psup->s_first_data_block);
}

int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre){ //EXPLICAR
   //Busca un fichero y devuelve el inodo en el que se encuentra (si no se encuentra devulve -1)
   for (int i=0; i<MAX_FICHEROS; i++) {
      if (strcmp(directorio[i].dir_nfich, nombre) == 0) {
      	 EXT_BLQ_INODOS *ext_blq_inodos;
         return i;
      }
   }
   return -1;
}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos){
   //Muestra los archivos guardados junto con su tamaño, inodo y bloques ocupados
	for(int i=1; i<MAX_FICHEROS; i++){
		 if(directorio[i].dir_inodo!=NULL_INODO && directorio[i].dir_nfich[0]!='\0'){
     		 printf("%s\tTamaño: %u \tInodo: %d\tBloques: ", directorio[i].dir_nfich, inodos->blq_inodos[directorio[i].dir_inodo].size_fichero, directorio[i].dir_inodo);
        	 for (int j=0;  j<MAX_NUMS_BLOQUE_INODO; j++) {
            	if (inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j] != NULL_BLOQUE) {
              	 printf("%d ", inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j]);
            }
         }
      printf("\n");}
   }
}

int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo){ //EXPLICAR
   //Renombra un archivo
   //Busca el inodo del archivo con el nombre a cambiar
   int indiceAntiguo = BuscaFich(directorio, inodos, nombreantiguo);
   //Si encontra el archivo al que cambiarle el nombre (indiceAntiguo diferente a -1) realiza el cambio 
   //de nombre despues de haber comprobado si no existe un archivo con el nombre nuevo
   if(indiceAntiguo!=-1){
      if(BuscaFich(directorio, inodos, nombrenuevo)!=-1){
         printf("ERROR: El archivo '%s' ya existe\n", nombrenuevo);
         return -1;
      }
      strncpy(directorio[indiceAntiguo].dir_nfich, nombrenuevo, LEN_NFICH-1);
      directorio[indiceAntiguo].dir_nfich[LEN_NFICH-1] = '\0';
      return 0;
   }
	printf("ERROR: No se encontró el archivo '%s'\n", nombreantiguo);
	return-1;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre){
   //Imprime el contenido del archivo indicado
   //Busca el inodo del archivo a imprimir
   int indice = BuscaFich(directorio, inodos, nombre);

   //Si no se encuentra se imprime el mensaje
   if(indice==-1){
		printf("ERROR: No se encontró el archivo '%s'\n", nombre);
		return -1;
   }
   EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[directorio[indice].dir_inodo];
   //Imprime el archivo entero recoriendolo caracter a caracter
   for(int i=0; i<MAX_NUMS_BLOQUE_INODO; i++){
   	if(inodo->i_nbloque[i]!=NULL_BLOQUE){
      fwrite(memdatos[inodo->i_nbloque[i]].dato, SIZE_BLOQUE, 1, stdout); //stdout salida estándar
     }
   }
   printf("\n");
   return 0;
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps,
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre,  FILE *fich){ //EXPLICAR
   //Borra el archivo indicado
   //Busca la existencia del arachivo a borrar
   int indice = BuscaFich(directorio, inodos, nombre);
   
   //Si lo enciuentra procede a la elimincion
   if(indice!=-1){
      EXT_SIMPLE_INODE *inodo = &inodos->blq_inodos[directorio[indice].dir_inodo];
      
      //Recorre el archivo borrando cada parte de los datos
      for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
         if (inodo->i_nbloque[i] != NULL_BLOQUE) {
            ext_bytemaps->bmap_bloques[inodo->i_nbloque[i]] = 0;
            ext_superblock->s_free_blocks_count++;
            inodo->i_nbloque[i] = NULL_BLOQUE;
         }
      }
      
      //Elimina la informacion de los metadatos de ese archivo y cambia la disponibilidad de los inodos/bloques (de 1 a 0)
      ext_bytemaps->bmap_inodos[directorio[indice].dir_inodo] = 0;
      ext_superblock->s_free_inodes_count++;
      inodo->size_fichero = 0;
      directorio[indice].dir_inodo = NULL_INODO;
      memset(directorio[indice].dir_nfich, 0, LEN_NFICH);
      return 0;
   }
   printf("ERROR: No se encontró el archivo '%s'\n", nombre);
   return -1;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, 
           EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, 
           char *nombredestino, FILE *fich) {
   // Buscar el archivo origen
   int indiceOrigen = BuscaFich(directorio, inodos, nombreorigen);
   if (indiceOrigen == -1) {
      printf("ERROR: No se encontró el archivo '%s'\n", nombreorigen);
      return -1;
   }

   // Verificar si el archivo destino ya existe
   if (BuscaFich(directorio, inodos, nombredestino) != -1) {
      printf("ERROR: El archivo '%s' ya existe\n", nombredestino);
      return -1;
   }

   // Encontrar el primer inodo libre
   int inodoLibre = -1;
   for (int i = 0; i < MAX_INODOS; i++) {
      if (ext_bytemaps->bmap_inodos[i] == 0) { // inodo libre
         inodoLibre = i;
         break; // Salir del bucle al encontrar uno libre
      }
   }

   if (inodoLibre == -1) {
      printf("ERROR: No hay inodos libres\n");
      return -1;
   }

   // Buscar una entrada libre en el directorio
   int nuevaEntrada = -1;
   for (int i = 0; i < MAX_FICHEROS; i++) {
      if (directorio[i].dir_inodo == NULL_INODO) { // Entrada libre
         nuevaEntrada = i;
         break; // Salir del bucle al encontrar una entrada libre
      }
   }

   if (nuevaEntrada == -1) {
      printf("ERROR: No hay espacio en el directorio\n");
      return -1;
   }

   // Copiar información del archivo origen al destino
   EXT_SIMPLE_INODE *inodoOrigen = &inodos->blq_inodos[directorio[indiceOrigen].dir_inodo];
   EXT_SIMPLE_INODE *inodoDestino = &inodos->blq_inodos[inodoLibre];

   inodoDestino->size_fichero = inodoOrigen->size_fichero;
   memset(inodoDestino->i_nbloque, NULL_BLOQUE, sizeof(inodoDestino->i_nbloque)); // Limpiar bloques

   for (int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++) {
      if (inodoOrigen->i_nbloque[i] != NULL_BLOQUE) {
         int nuevoBloque = -1;
         for (int j = 0; j < MAX_BLOQUES_DATOS; j++) {
            if (ext_bytemaps->bmap_bloques[j] == 0) { // Bloque libre
               nuevoBloque = j;
               ext_bytemaps->bmap_bloques[j] = 1; // Marcar bloque como ocupado
               break;
            }
         }

         if (nuevoBloque == -1) {
            printf("ERROR: No hay bloques libres suficientes\n");
            return -1;
         }

         memcpy(&memdatos[nuevoBloque], &memdatos[inodoOrigen->i_nbloque[i]], SIZE_BLOQUE);
         inodoDestino->i_nbloque[i] = nuevoBloque;
      }
   }

   // Crear nueva entrada en el directorio
   ext_bytemaps->bmap_inodos[inodoLibre] = 1;
   strcpy(directorio[nuevaEntrada].dir_nfich, nombredestino);
   directorio[nuevaEntrada].dir_inodo = inodoLibre;

   return 0;
}

void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) { //EXPLICAR
   //Sitúa el puntero de lectura/escritura de fich en la seccion de inodos (bloque 2)
   fseek(fich, SIZE_BLOQUE * 2, SEEK_SET);

   //Escribe 1 elemento, de tamaño SIZE_BLOQUE, desde inodo en fich
   fwrite(inodos, SIZE_BLOQUE, 1, fich);


   //Sitúa el puntero de lectura/escritura de fich en la seccion de directorio (bloque 3)
   fseek(fich, SIZE_BLOQUE * 3, SEEK_SET);

   //Escribe 1 elemento, de tamaño SIZE_BLOQUE, desde directorio en fich
   fwrite(directorio, SIZE_BLOQUE, 1, fich);
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
   //Sitúa el puntero de lectura/escritura de fich en la seccion de Bytemaps (bloque 1)
   fseek(fich, SIZE_BLOQUE, SEEK_SET);

   //Escribe 1 elemento, de tamaño SIZE_BLOQUE, desde directorio en fich
   fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) { //EXPLICAR
   //Sitúa el puntero de lectura/escritura de fich en la seccion inicial/SuperBloque (bloque 0)
   fseek(fich, 0, SEEK_SET); 

   //Escribe 1 elemento, de tamaño SIZE_BLOQUE, desde ext_superblock en fich
   fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
   //Sitúa el puntero de lectura/escritura de fich en la seccion de datos (bloque 4)
   fseek(fich, SIZE_BLOQUE * PRIM_BLOQUE_DATOS, SEEK_SET);

   //Escribe MAX_BLOQUES_DATOS elementos, de tamaño SIZE_BLOQUE, desde memdatos en fich
   fwrite(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);
}
