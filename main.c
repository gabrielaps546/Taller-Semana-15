#include "funciones.h"
#include <string.h>

int main(void)
{
    Zona zonas[MAX_ZONAS];
    PrediccionCompleta pred[MAX_ZONAS];
    FactoresClimaticos clima = {25,10,180,60,1013,3,0};
    int nzonas=0;

    VerificarOCrearArchivo("datos.txt");
    VerificarOCrearArchivo("reporte.txt");
    cargarZonas(zonas,&nzonas,"datos.txt");

    int op;
    do{
        printf("\n=== SISTEMA DE MONITOREO DE CALIDAD DEL AIRE === \n");
        printf("1. Ingresar datos de una zona \n");
        printf("2. Mostrar ICA de todas las zonas \n");
        printf("3. Calcular promedios historicos \n");
        printf("4. Predecir contaminacion (24h) \n");
        printf("5. Emitir alertas preventivas \n");
        printf("6. Generar recomendaciones \n");
        printf("7. Exportar reporte completo \n");
        printf("8. Salir \n");
        printf("Seleccione una opcion: ");
        if(scanf("%d",&op)!=1) op=8;
        while(getchar()!='\n');

        switch(op){
        case 1:
            if (nzonas>=MAX_ZONAS){puts("Máximo de zonas alcanzado");break;}
            if (ingresarDatosZona(&zonas[nzonas],"datos.txt")){
                guardarZonaIndividual(&zonas[nzonas],"datos.txt");
                ++nzonas;
            }
            break;

        case 2:
            if(nzonas) mostrarICATodasZonas(zonas,nzonas);
            else printf("No hay zonas.");
            break;

        case 3:
            if(nzonas) calcularPromediosHistoricos(zonas,nzonas);
            else printf("No hay zonas.");
            break;

        case 4:
            if(nzonas){
                predecirContaminacionAvanzada(zonas,nzonas,&clima,pred);
                mostrarPrediccionesICA(zonas,pred,nzonas);
            }else printf("No hay zonas.");
            break;

        case 5:
            if(nzonas){
                predecirContaminacionAvanzada(zonas,nzonas,&clima,pred);
                emitirAlertas(zonas,nzonas,pred);
            }else printf("No hay zonas.");
            break;

        case 6:
            if(nzonas){
                predecirContaminacionAvanzada(zonas,nzonas,&clima,pred);
                generarRecomendaciones(zonas,nzonas,pred);
            }else printf("No hay zonas.");
            break;

        case 7:
            if(nzonas){
                predecirContaminacionAvanzada(zonas,nzonas,&clima,pred);
                exportarReporte(zonas,nzonas,"reporte.txt",pred);
            }else printf("No hay zonas.");
            break;
        }
    }while(op!=8);

    return 0;
}