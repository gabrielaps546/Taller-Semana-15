#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <stdio.h>

#define MAX_ZONAS       10
#define MAX_NOMBRE      50
#define DIAS_HISTORICO  60

/* Límites OMS */
#define LIMITE_CO 10
#define LIMITE_SO2 40
#define LIMITE_NO2 25
#define LIMITE_PM25 15
#define LIMITE_O3 60
#define LIMITE_PM10 45
#define LIMITE_CO2 5000

typedef struct {
    char  nombre[MAX_NOMBRE];
    float co_actual,   so2_actual,  no2_actual,
          pm25_actual, pm10_actual, o3_actual,
          co2_actual,  temperatura, viento,  humedad;
    float co_hist[DIAS_HISTORICO],   so2_hist[DIAS_HISTORICO],
          no2_hist[DIAS_HISTORICO],  pm25_hist[DIAS_HISTORICO],
          pm10_hist[DIAS_HISTORICO], o3_hist[DIAS_HISTORICO],
          co2_hist[DIAS_HISTORICO];
} Zona;

typedef struct {
    float temperatura, velocidad_viento, direccion_viento,
          humedad, presion_atmosferica;
    int   dia_semana;  /* 1-lunes … 7-domingo   */
    int   es_festivo;  /* 0 o 1                 */
} FactoresClimaticos;

typedef struct {
    float co_pred, so2_pred, no2_pred,
          pm25_pred, pm10_pred, o3_pred, co2_pred;
    float ica_predicho;
    char  categoria_predicha[50],
          color_predicho[20],
          contaminante_dominante_pred[10];
    float confianza; /* 0-1 */
} PrediccionCompleta;

/* Fechas y persistencia */
void   obtenerFechaActual(char *f);
int    verificarEntradaHoy(const char *file,const char*zona,const char*fecha);
void   VerificarOCrearArchivo(const char *file);

/* Carga / guardado */
void cargarZonas (Zona z[], int *n, const char *file);
void guardarZonas(Zona z[], int  n, const char *file);
void guardarZonaIndividual(Zona *z, const char *file);

/* Ingreso */
int ingresarDatosZona(Zona *z, const char *file);

/* Estadísticas y utilidades */
void calcularPromediosHistoricos(Zona z[], int n);
void mostrarICATodasZonas(Zona z[], int n);

/* ICA */
float calcularICAIndividual(float C, float bp[], int n);
float calcularICAZona     (Zona *z,char*dom,char*cat,char*col);

/* Predicción */
float factorDiaSemana(int d,const char*c);               /* helpers */
float factorClimatico(FactoresClimaticos*,const char*c);
float calcularTendencia(float x[],int dias);

void predecirContaminacionAvanzada(Zona z[], int n,
        FactoresClimaticos *clima, PrediccionCompleta p[]);
void mostrarPrediccionesICA(Zona z[], PrediccionCompleta p[], int n);
void emitirAlertas(Zona zonas[], int cantidad, PrediccionCompleta predicciones[]);
void generarRecomendaciones(Zona zonas[], int cantidad, PrediccionCompleta predicciones[]);

/* Reporte */
void exportarReporte(Zona z[], int n,const char*file,PrediccionCompleta p[]);
#endif