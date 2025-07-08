#include "funciones.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define MAX_ZONAS 10
#define MAX_NOMBRE 50
#define DIAS_HISTORICO 60

#define LIMITE_CO 10
#define LIMITE_SO2 40
#define LIMITE_NO2 25
#define LIMITE_PM25 15
#define LIMITE_O3 60
#define LIMITE_PM10 45
#define LIMITE_CO2 5000

// Rangos de concentración para cada contaminante (puntos de corte)
// PM2.5 (μg/m³)
float PM25_BREAKPOINTS[] = {0, 12, 35.4, 55.4, 150.4, 250.4, 350.4, 500.4};
// PM10 (μg/m³)
float PM10_BREAKPOINTS[] = {0, 54, 154, 254, 354, 424, 504, 604};
// CO (ppm)
float CO_BREAKPOINTS[] = {0, 4.4, 9.4, 12.4, 15.4, 30.4, 40.4, 50.4};
// SO2 (ppb)
float SO2_BREAKPOINTS[] = {0, 35, 75, 185, 304, 604, 804, 1004};
// NO2 (ppb)
float NO2_BREAKPOINTS[] = {0, 53, 100, 360, 649, 1249, 1649, 2049};
// O3 (ppb) - 8 horas
float O3_BREAKPOINTS[] = {0, 54, 70, 85, 105, 200, 300, 400};

// Valores del ICA correspondientes a cada rango
float ICA_VALUES[] = {0, 50, 100, 150, 200, 300, 400, 500};


// Categorías del ICA
char* ICA_CATEGORIES[] = {
    "Buena",
    "Moderada", 
    "Danina para grupos sensibles",
    "Danina para la salud",
    "Muy danina para la salud",
    "Peligrosa",
    "Extremadamente peligrosa"
};

// Colores asociados (para reportes)
char* ICA_COLORS[] = {
    "Verde",
    "Amarillo",
    "Naranja", 
    "Rojo",
    "Purpura",
    "Granate",
    "Granate"
};

void VerificarOCrearArchivo(const char *fn)
{
    FILE *f = fopen(fn,"r");
    if (f) { fclose(f); return; }      /* ya existe */
    f = fopen(fn,"w");
    if (!f) return;
    fprintf(f,"FECHA,ZONA,PM25,NO2,SO2,CO,PM10,O3,CO2,TEMP,VIENTO,HUMEDAD\n");
    fclose(f);
}

// Función para obtener la fecha actual en formato YYYY-MM-DD
void obtenerFechaActual(char* fecha) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(fecha, "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

// Función para verificar si ya existe una entrada para la fecha actual
int verificarEntradaHoy(const char* filename, const char* zonaNombre, const char* fechaHoy) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0; // Si no existe el archivo, no hay entradas
    
    char linea[512];
    fgets(linea, sizeof(linea), file); // Saltar encabezado
    
    while (fgets(linea, sizeof(linea), file)) {
        char fecha[20], zona[MAX_NOMBRE];
        int n = sscanf(linea, "%19[^,],%49[^,]", fecha, zona);
        if (n == 2) {
            if (strcmp(fecha, fechaHoy) == 0 && strcmp(zona, zonaNombre) == 0) {
                fclose(file);
                return 1; // Ya existe una entrada para hoy
            }
        }
    }
    fclose(file);
    return 0; // No existe entrada para hoy
}

// Cargar datos de zonas desde archivo TXT
void cargarZonas(Zona zonas[], int *cantidad, const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        *cantidad = 0;
        return;
    }
    char linea[512];
    fgets(linea, sizeof(linea), file); // Saltar encabezado

    int numZonas = 0;
    while (fgets(linea, sizeof(linea), file)) {
        char fecha[20], zonaNombre[MAX_NOMBRE];
        float pm25, no2, so2, co, pm10, o3, co2, temp, viento, humedad;

        // Modificado para leer todos los contaminantes
        int n = sscanf(linea, "%19[^,],%49[^,],%f,%f,%f,%f,%f,%f,%f,%f,%f,%f",
            fecha, zonaNombre, &pm25, &no2, &so2, &co, &pm10, &o3, &co2, &temp, &viento, &humedad);
        if (n != 12) continue; // Ahora esperamos 12 campos

        int idx = -1;
        for (int i = 0; i < numZonas; i++) {
            if (strcmp(zonas[i].nombre, zonaNombre) == 0) {
                idx = i;
                break;
            }
        }
        if (idx == -1) {
            idx = numZonas++;
            strncpy(zonas[idx].nombre, zonaNombre, MAX_NOMBRE);
            zonas[idx].nombre[MAX_NOMBRE-1] = '\0';
            
            // Inicializar todos los contaminantes actuales
            zonas[idx].co_actual = co;
            zonas[idx].so2_actual = so2;
            zonas[idx].no2_actual = no2;
            zonas[idx].pm25_actual = pm25;
            zonas[idx].pm10_actual = pm10;
            zonas[idx].o3_actual = o3;
            zonas[idx].co2_actual = co2;
            zonas[idx].temperatura = temp;
            zonas[idx].viento = viento;
            zonas[idx].humedad = humedad;
            
            // Inicializar históricos
            for (int j = 0; j < DIAS_HISTORICO; j++) {
                zonas[idx].co_hist[j] = 0;
                zonas[idx].so2_hist[j] = 0;
                zonas[idx].no2_hist[j] = 0;
                zonas[idx].pm25_hist[j] = 0;
                zonas[idx].pm10_hist[j] = 0;
                zonas[idx].o3_hist[j] = 0;
                zonas[idx].co2_hist[j] = 0;
            }
        }
        
        // Desplazar históricos
        for (int j = DIAS_HISTORICO-1; j > 0; j--) {
            zonas[idx].co_hist[j] = zonas[idx].co_hist[j-1];
            zonas[idx].so2_hist[j] = zonas[idx].so2_hist[j-1];
            zonas[idx].no2_hist[j] = zonas[idx].no2_hist[j-1];
            zonas[idx].pm25_hist[j] = zonas[idx].pm25_hist[j-1];
            zonas[idx].pm10_hist[j] = zonas[idx].pm10_hist[j-1];
            zonas[idx].o3_hist[j] = zonas[idx].o3_hist[j-1];
            zonas[idx].co2_hist[j] = zonas[idx].co2_hist[j-1];
        }
        
        // Agregar valores actuales al histórico
        zonas[idx].co_hist[0] = co;
        zonas[idx].so2_hist[0] = so2;
        zonas[idx].no2_hist[0] = no2;
        zonas[idx].pm25_hist[0] = pm25;
        zonas[idx].pm10_hist[0] = pm10;
        zonas[idx].o3_hist[0] = o3;
        zonas[idx].co2_hist[0] = co2;
        
        // Actualizar valores actuales
        zonas[idx].co_actual = co;
        zonas[idx].so2_actual = so2;
        zonas[idx].no2_actual = no2;
        zonas[idx].pm25_actual = pm25;
        zonas[idx].pm10_actual = pm10;
        zonas[idx].o3_actual = o3;
        zonas[idx].co2_actual = co2;
        zonas[idx].temperatura = temp;
        zonas[idx].viento = viento;
        zonas[idx].humedad = humedad;
    }
    *cantidad = numZonas;
    fclose(file);
}

// Ingresar datos de una zona y comparar con histórico (modificada)
int ingresarDatosZona(Zona* zona, const char* filename) {
    char fechaHoy[20];
    obtenerFechaActual(fechaHoy);
    
    printf("Nombre de la zona: ");
    fgets(zona->nombre, MAX_NOMBRE, stdin);
    zona->nombre[strcspn(zona->nombre, "\n")] = '\0';
    
    // Verificar si ya existe una entrada para hoy
    if (verificarEntradaHoy(filename, zona->nombre, fechaHoy)) {
        printf("ERROR: Ya existe una entrada para la zona '%s' en la fecha de hoy (%s).\n", 
               zona->nombre, fechaHoy);
        printf("No se pueden ingresar datos duplicados para el mismo día.\n");
        return 0; // Retorna 0 para indicar que no se pudo ingresar
    }
    
    // Solicitar todos los contaminantes
    printf("CO actual (ppm): ");
    if (scanf("%f", &zona->co_actual) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n'); // limpiar buffer
        scanf("%f", &zona->co_actual);
    }

    printf("SO2 actual (ug/m3): ");
    if (scanf("%f", &zona->so2_actual) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->so2_actual);
    }

    printf("NO2 actual (ug/m3): ");
    if (scanf("%f", &zona->no2_actual) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->no2_actual);
    }

    printf("PM2.5 actual (ug/m3): ");
    if (scanf("%f", &zona->pm25_actual) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->pm25_actual);
    }

    printf("PM10 actual (ug/m3): ");
    if (scanf("%f", &zona->pm10_actual) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->pm10_actual);
    }

    printf("O3 actual (ug/m3): ");
    if (scanf("%f", &zona->o3_actual) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->o3_actual);
    }

    printf("CO2 actual (ppm): ");
    if (scanf("%f", &zona->co2_actual) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->co2_actual);
    }

    printf("Temperatura (°C): ");
    if (scanf("%f", &zona->temperatura) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->temperatura);
    }

    printf("Velocidad del viento (km/h): ");
    if (scanf("%f", &zona->viento) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->viento);
    }

    printf("Humedad (%%): ");
    if (scanf("%f", &zona->humedad) != 1) {
        printf("Ingrese un valor numerico: ");
        while (getchar() != '\n');
        scanf("%f", &zona->humedad);
    }
    
    // Desplazar históricos y agregar el valor actual
    for (int i = DIAS_HISTORICO-1; i > 0; i--) {
        zona->co_hist[i] = zona->co_hist[i-1];
        zona->so2_hist[i] = zona->so2_hist[i-1];
        zona->no2_hist[i] = zona->no2_hist[i-1];
        zona->pm25_hist[i] = zona->pm25_hist[i-1];
        zona->pm10_hist[i] = zona->pm10_hist[i-1];
        zona->o3_hist[i] = zona->o3_hist[i-1];
        zona->co2_hist[i] = zona->co2_hist[i-1];
    }
    
    // Agregar valores actuales al histórico
    zona->co_hist[0] = zona->co_actual;
    zona->so2_hist[0] = zona->so2_actual;
    zona->no2_hist[0] = zona->no2_actual;
    zona->pm25_hist[0] = zona->pm25_actual;
    zona->pm10_hist[0] = zona->pm10_actual;
    zona->o3_hist[0] = zona->o3_actual;
    zona->co2_hist[0] = zona->co2_actual;
    
    while(getchar() != '\n'); // Limpiar buffer
    
    printf("Datos ingresados exitosamente para la fecha: %s\n", fechaHoy);
    return 1; // Retorna 1 para indicar éxito
}

// Función adicional para guardar una sola zona (para usar después de ingresar datos)
void guardarZonaIndividual(Zona* zona, const char* filename) {
    FILE *file = fopen(filename, "a");
    if (!file) return;
    
    char fechaHoy[20];
    obtenerFechaActual(fechaHoy);
    
    // Guardar fecha actual y todos los contaminantes actuales
    fprintf(file, "%s,%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
        fechaHoy, zona->nombre, zona->pm25_actual, zona->no2_actual, 
        zona->so2_actual, zona->co_actual, zona->pm10_actual, 
        zona->o3_actual, zona->co2_actual, zona->temperatura, 
        zona->viento, zona->humedad);
    
    fclose(file);
}

// Emitir alertas si la predicción supera límites
void emitirAlertas(Zona z[], int n, PrediccionCompleta p[])
{
    puts("\nAlertas preventivas para las proximas 24h:");
    for (int i=0;i<n;++i){
        int alerta = 0;
        if (p[i].co_pred   > LIMITE_CO)   {printf("Zona %s: ALERTA CO\n",  z[i].nombre); alerta=1;}
        if (p[i].so2_pred  > LIMITE_SO2)  {printf("Zona %s: ALERTA SO2\n", z[i].nombre); alerta=1;}
        if (p[i].no2_pred  > LIMITE_NO2)  {printf("Zona %s: ALERTA NO2\n", z[i].nombre); alerta=1;}
        if (p[i].pm25_pred > LIMITE_PM25) {printf("Zona %s: ALERTA PM2.5\n",z[i].nombre); alerta=1;}
        if (p[i].pm10_pred > LIMITE_PM10) {printf("Zona %s: ALERTA PM10\n",z[i].nombre); alerta=1;}
        if (p[i].o3_pred   > LIMITE_O3)   {printf("Zona %s: ALERTA O3\n",  z[i].nombre); alerta=1;}
        if (p[i].co2_pred  > LIMITE_CO2)  {printf("Zona %s: ALERTA CO2\n", z[i].nombre); alerta=1;}
        if (!alerta) printf("Zona %s: Sin alertas\n", z[i].nombre);
    }
}

// Generar recomendaciones
void generarRecomendaciones(Zona z[], int n, PrediccionCompleta p[])
{
    char nombre[MAX_NOMBRE];
    printf("Ingrese el nombre de la zona: ");
    fgets(nombre,MAX_NOMBRE,stdin);
    nombre[strcspn(nombre,"\n")] = '\0';

    // Validación: verificar si la zona existe
    int encontrada = 0;
    for(int i=0;i<n;++i){
        if(strcmp(z[i].nombre,nombre)==0){
            encontrada = 1;
            break;
        }
    }
    if (!encontrada) {
        puts("No existen recomendaciones para la zona");
        return;
    }

    for(int i=0;i<n;++i){
        if(strcmp(z[i].nombre,nombre)==0){
            int riesgo = (p[i].co_pred   > LIMITE_CO   || z[i].co_actual   > LIMITE_CO)   ||
                          (p[i].so2_pred  > LIMITE_SO2  || z[i].so2_actual  > LIMITE_SO2)  ||
                          (p[i].no2_pred  > LIMITE_NO2  || z[i].no2_actual  > LIMITE_NO2)  ||
                          (p[i].pm25_pred > LIMITE_PM25 || z[i].pm25_actual > LIMITE_PM25) ||
                          (p[i].pm10_pred > LIMITE_PM10 || z[i].pm10_actual > LIMITE_PM10) ||
                          (p[i].o3_pred   > LIMITE_O3   || z[i].o3_actual   > LIMITE_O3)   ||
                          (p[i].co2_pred  > LIMITE_CO2  || z[i].co2_actual  > LIMITE_CO2);

            if (!riesgo){
                puts("Niveles dentro de limites aceptables.");
                return;
            }

            FILE *f=fopen("recomendaciones.txt","r");
            if(!f){puts("No se pudo abrir recomendaciones.txt");return;}
            char linea[512];
            printf("Recomendaciones para la zona %s:\n",nombre);
            while(fgets(linea,sizeof linea,f)){
                if(strstr(linea,nombre)) fputs(linea,stdout);
            }
            fclose(f);
            return;
        }
    }
}

// Calcular promedios históricos de los últimos 30 días
void calcularPromediosHistoricos(Zona z[], int n)
{
    puts("\nPromedios historicos (ultimos 30 días):");
    for(int i=0;i<n;++i){
        float prom_co=0,prom_so2=0,prom_no2=0,prom_pm25=0,
              prom_pm10=0,prom_o3=0,prom_co2=0;

        // Contar registros válidos (asumimos que un registro es válido si al menos uno de los contaminantes es distinto de cero)
        int registros_validos = 0;
        for(int j=0;j<DIAS_HISTORICO;++j){
            if (z[i].co_hist[j]!=0 || z[i].so2_hist[j]!=0 || z[i].no2_hist[j]!=0 ||
                z[i].pm25_hist[j]!=0 || z[i].pm10_hist[j]!=0 || z[i].o3_hist[j]!=0 || z[i].co2_hist[j]!=0) {
                registros_validos++;
            } else {
                break; // asumimos que los registros válidos están al principio
            }
        }
        int dias = 30;
        if (registros_validos < dias) {
            dias = registros_validos;
            printf("\nZona: %s\nSolo existen %d registros historicos. El promedio se calcula con esos registros.\n",z[i].nombre,dias);
        } else {
            printf("\nZona: %s\n",z[i].nombre);
        }
        if (dias == 0) {
            printf("No hay datos historicos para esta zona.\n");
            continue;
        }
        
        for(int j=0;j<dias;++j){
            prom_co   += z[i].co_hist[j];
            prom_so2  += z[i].so2_hist[j];
            prom_no2  += z[i].no2_hist[j];
            prom_pm25 += z[i].pm25_hist[j];
            prom_pm10 += z[i].pm10_hist[j];
            prom_o3   += z[i].o3_hist[j];
            prom_co2  += z[i].co2_hist[j];
        }
        prom_co   /= dias;
        prom_so2  /= dias;
        prom_no2  /= dias;
        prom_pm25 /= dias;
        prom_pm10 /= dias;
        prom_o3   /= dias;
        prom_co2  /= dias;

        printf(" CO:   %.2f ppm  (%s)\n",prom_co,   prom_co  >LIMITE_CO   ?"ALTO":"OK");
        printf(" SO2:  %.2f ppb  (%s)\n",prom_so2,  prom_so2 >LIMITE_SO2 ?"ALTO":"OK");
        printf(" NO2:  %.2f ppb  (%s)\n",prom_no2,  prom_no2 >LIMITE_NO2 ?"ALTO":"OK");
        printf(" PM2.5 %.2f ug/m3 (%s)\n",prom_pm25,prom_pm25>LIMITE_PM25?"ALTO":"OK");
        printf(" PM10  %.2f ug/m3 (%s)\n",prom_pm10,prom_pm10>LIMITE_PM10?"ALTO":"OK");
        printf(" O3:   %.2f ppb  (%s)\n",prom_o3,   prom_o3  >LIMITE_O3  ?"ALTO":"OK");
        printf(" CO2:  %.2f ppm  (%s)\n",prom_co2,  prom_co2 >LIMITE_CO2 ?"ALTO":"OK");
    }
}

// Función para calcular el ICA de un contaminante individual
float calcularICAIndividual(float concentracion, float breakpoints[], int num_breakpoints) {
    // Encontrar el rango donde se encuentra la concentración
    for (int i = 0; i < num_breakpoints - 1; i++) {
        if (concentracion >= breakpoints[i] && concentracion <= breakpoints[i + 1]) {
            // Aplicar la fórmula del ICA
            float Ip = ICA_VALUES[i + 1];      // ICA superior
            float Ii = ICA_VALUES[i];          // ICA inferior
            float Cp = breakpoints[i + 1];     // Concentración superior
            float Ci = breakpoints[i];         // Concentración inferior
            float C = concentracion;           // Concentración actual
            
            // Fórmula: ICA = ((Ip - Ii) / (Cp - Ci)) * (C - Ci) + Ii
            float ica = ((Ip - Ii) / (Cp - Ci)) * (C - Ci) + Ii;
            return ica;
        }
    }
    
    // Si la concentración excede el rango máximo, devolver 500+
    if (concentracion > breakpoints[num_breakpoints - 1]) {
        return 500.0;
    }
    
    return 0.0; // Concentración muy baja
}

// Función principal para calcular el ICA de una zona
float calcularICAZona(Zona* zona, char* contaminante_dominante, char* categoria, char* color) {
    float ica_values[6]; // Array para almacenar ICA de cada contaminante
    char* contaminantes[] = {"PM2.5", "PM10", "CO", "SO2", "NO2", "O3"};
    
    // Calcular ICA para cada contaminante
    ica_values[0] = calcularICAIndividual(zona->pm25_actual, PM25_BREAKPOINTS, 8);
    ica_values[1] = calcularICAIndividual(zona->pm10_actual, PM10_BREAKPOINTS, 8);
    ica_values[2] = calcularICAIndividual(zona->co_actual, CO_BREAKPOINTS, 8);
    ica_values[3] = calcularICAIndividual(zona->so2_actual, SO2_BREAKPOINTS, 8);
    ica_values[4] = calcularICAIndividual(zona->no2_actual, NO2_BREAKPOINTS, 8);
    ica_values[5] = calcularICAIndividual(zona->o3_actual, O3_BREAKPOINTS, 8);
    
    // Encontrar el ICA más alto (contaminante dominante)
    float ica_final = 0;
    int indice_dominante = 0;
    
    for (int i = 0; i < 6; i++) {
        if (ica_values[i] > ica_final) {
            ica_final = ica_values[i];
            indice_dominante = i;
        }
    }
    
    // Determinar categoría y color
    int categoria_index = 0;
    if (ica_final <= 50) categoria_index = 0;
    else if (ica_final <= 100) categoria_index = 1;
    else if (ica_final <= 150) categoria_index = 2;
    else if (ica_final <= 200) categoria_index = 3;
    else if (ica_final <= 300) categoria_index = 4;
    else if (ica_final <= 400) categoria_index = 5;
    else categoria_index = 6;
    
    // Copiar resultados a los parámetros de salida
    strcpy(contaminante_dominante, contaminantes[indice_dominante]);
    strcpy(categoria, ICA_CATEGORIES[categoria_index]);
    strcpy(color, ICA_COLORS[categoria_index]);
    
    return ica_final;
}

// Función para mostrar el ICA de todas las zonas
void mostrarICATodasZonas(Zona zonas[], int cantidad) {
    printf("\n=== ÍNDICE DE CALIDAD DEL AIRE (ICA) ===\n\n");
    
    for (int i = 0; i < cantidad; i++) {
        char contaminante_dominante[10];
        char categoria[50];
        char color[20];
        
        float ica = calcularICAZona(&zonas[i], contaminante_dominante, categoria, color);
        
        printf("Zona: %s\n", zonas[i].nombre);
        printf("ICA: %.0f (%s - %s)\n", ica, categoria, color);
        printf("Contaminante dominante: %s\n", contaminante_dominante);
        
        // Mostrar recomendaciones de salud
        printf("Recomendaciones: ");
        if (ica <= 50) {
            printf("Calidad del aire satisfactoria. Actividades al aire libre sin restricciones.\n");
        } else if (ica <= 100) {
            printf("Calidad del aire aceptable. Personas sensibles pueden experimentar sintomas menores.\n");
        } else if (ica <= 150) {
            printf("Grupos sensibles deben reducir actividades prolongadas al aire libre.\n");
        } else if (ica <= 200) {
            printf("Todos pueden experimentar efectos en la salud. Limitar actividades al aire libre.\n");
        } else if (ica <= 300) {
            printf("Advertencia de salud: todos pueden experimentar efectos serios.\n");
        } else {
            printf("Emergencia de salud: todos pueden experimentar efectos graves.\n");
        }
        
        printf("----------------------------------------\n");
    }
}

// Función para calcular tendencia (creciente, decreciente, estable)
float calcularTendencia(float datos[], int dias) {
    if (dias < 7) return 0.0; // Necesitamos al menos 7 días
    
    float suma_reciente = 0, suma_anterior = 0;
    int mitad = dias / 2;
    
    // Promedio de la mitad más reciente
    for (int i = 0; i < mitad; i++) {
        suma_reciente += datos[i];
    }
    
    // Promedio de la mitad anterior
    for (int i = mitad; i < dias; i++) {
        suma_anterior += datos[i];
    }
    
    float prom_reciente = suma_reciente / mitad;
    float prom_anterior = suma_anterior / (dias - mitad);
    
    // Retorna la diferencia porcentual
    return (prom_reciente - prom_anterior) / prom_anterior;
}

// Función para calcular factor estacional (día de la semana)
float factorDiaSemana(int dia_semana, const char* contaminante) {
    // Factores basados en patrones típicos de contaminación
    // Lunes a Viernes: más tráfico y actividad industrial
    // Fines de semana: menos actividad
    
    if (strcmp(contaminante, "NO2") == 0 || strcmp(contaminante, "CO") == 0) {
        // Contaminantes relacionados con tráfico
        if (dia_semana >= 2 && dia_semana <= 6) return 1.2; // Martes-Sábado
        if (dia_semana == 1 || dia_semana == 7) return 0.8; // Lunes y Domingo
    }
    
    if (strcmp(contaminante, "SO2") == 0) {
        // Relacionado con industria
        if (dia_semana >= 1 && dia_semana <= 5) return 1.1; // Lunes-Viernes
        else return 0.9; // Fines de semana
    }
    
    return 1.0; // Factor neutro para otros contaminantes
}

// Función para calcular factor climático
float factorClimatico(FactoresClimaticos* clima, const char* contaminante) {
    float factor = 1.0;
    
    // Factor de viento (dispersión)
    if (clima->velocidad_viento > 15) factor *= 0.7; // Viento fuerte dispersa
    else if (clima->velocidad_viento < 5) factor *= 1.3; // Viento débil concentra
    
    // Factor de temperatura
    if (strcmp(contaminante, "O3") == 0) {
        // Ozono se forma más con altas temperaturas
        if (clima->temperatura > 25) factor *= 1.4;
        else if (clima->temperatura < 15) factor *= 0.8;
    }
    
    // Factor de humedad para material particulado
    if (strcmp(contaminante, "PM25") == 0 || strcmp(contaminante, "PM10") == 0) {
        if (clima->humedad > 70) factor *= 1.2; // Alta humedad favorece PM
        else if (clima->humedad < 30) factor *= 0.9;
    }
    
    // Factor de presión atmosférica (inversión térmica)
    if (clima->presion_atmosferica > 1020) factor *= 1.2; // Alta presión puede causar inversión
    
    // Factores específicos para CO2
    if (strcmp(contaminante, "CO2") == 0) {
        // CO2 es menos afectado por el clima, pero la inversión térmica puede concentrarlo
        if (clima->presion_atmosferica > 1020 && clima->velocidad_viento < 5) factor *= 1.1;
    }
    
    return factor;
}

// Función de predicción avanzada
void predecirContaminacionAvanzada(Zona zonas[], int cantidad, FactoresClimaticos* clima_actual,PrediccionCompleta predicciones[]) {
    
    for (int i = 0; i < cantidad; i++) {
        // Contar registros válidos (al menos un contaminante distinto de cero)
        int registros_validos = 0;
        for (int j = 0; j < DIAS_HISTORICO; ++j) {
            if (zonas[i].co_hist[j]!=0 || zonas[i].so2_hist[j]!=0 || zonas[i].no2_hist[j]!=0 ||
                zonas[i].pm25_hist[j]!=0 || zonas[i].pm10_hist[j]!=0 || zonas[i].o3_hist[j]!=0 || zonas[i].co2_hist[j]!=0) {
                registros_validos++;
            } else {
                break; // asumimos que los registros válidos están al principio
            }
        }
        if (registros_validos == 0) {
            // No hay datos históricos, omitir predicción
            predicciones[i].co_pred = 0;
            predicciones[i].so2_pred = 0;
            predicciones[i].no2_pred = 0;
            predicciones[i].pm25_pred = 0;
            predicciones[i].pm10_pred = 0;
            predicciones[i].o3_pred = 0;
            predicciones[i].co2_pred = 0;
            predicciones[i].ica_predicho = 0;
            strcpy(predicciones[i].categoria_predicha, "Sin datos");
            strcpy(predicciones[i].color_predicho, "Sin datos");
            strcpy(predicciones[i].contaminante_dominante_pred, "Sin datos");
            predicciones[i].confianza = 0;
            continue;
        }
        if (registros_validos < 7) {
            printf("Advertencia: Zona %s solo tiene %d registros historicos para la prediccion.\n", zonas[i].nombre, registros_validos);
        }
        float co_hist[DIAS_HISTORICO], so2_hist[DIAS_HISTORICO];
        float no2_hist[DIAS_HISTORICO], pm25_hist[DIAS_HISTORICO];
        float pm10_hist[DIAS_HISTORICO], o3_hist[DIAS_HISTORICO];
        float co2_hist[DIAS_HISTORICO];
        // Copiar datos históricos
        for (int j = 0; j < registros_validos; j++) {
            co_hist[j] = zonas[i].co_hist[j];
            so2_hist[j] = zonas[i].so2_hist[j];
            no2_hist[j] = zonas[i].no2_hist[j];
            pm25_hist[j] = zonas[i].pm25_hist[j];
            pm10_hist[j] = zonas[i].pm10_hist[j];
            o3_hist[j] = zonas[i].o3_hist[j];
            co2_hist[j] = zonas[i].co2_hist[j];
        }
        // Calcular tendencias
        float tendencia_co = calcularTendencia(co_hist, registros_validos);
        float tendencia_so2 = calcularTendencia(so2_hist, registros_validos);
        float tendencia_no2 = calcularTendencia(no2_hist, registros_validos);
        float tendencia_pm25 = calcularTendencia(pm25_hist, registros_validos);
        float tendencia_pm10 = calcularTendencia(pm10_hist, registros_validos);
        float tendencia_o3 = calcularTendencia(o3_hist, registros_validos);
        float tendencia_co2 = calcularTendencia(co2_hist, registros_validos);
        // Calcular promedio ponderado base
        float peso_total = 0, pred_co = 0, pred_so2 = 0, pred_no2 = 0, pred_pm25 = 0;
        float pred_pm10 = 0, pred_o3 = 0, pred_co2 = 0;
        for (int j = 0; j < registros_validos; j++) {
            float peso = exp(-j * 0.05); // Decaimiento exponencial
            pred_co += co_hist[j] * peso;
            pred_so2 += so2_hist[j] * peso;
            pred_no2 += no2_hist[j] * peso;
            pred_pm25 += pm25_hist[j] * peso;
            pred_pm10 += pm10_hist[j] * peso;
            pred_o3 += o3_hist[j] * peso;
            pred_co2 += co2_hist[j] * peso;
            peso_total += peso;
        }
        pred_co /= peso_total;
        pred_so2 /= peso_total;
        pred_no2 /= peso_total;
        pred_pm25 /= peso_total;
        pred_pm10 /= peso_total;
        pred_o3 /= peso_total;
        pred_co2 /= peso_total;
        // Aplicar factores de tendencia
        pred_co *= (1 + tendencia_co * 0.3);
        pred_so2 *= (1 + tendencia_so2 * 0.3);
        pred_no2 *= (1 + tendencia_no2 * 0.3);
        pred_pm25 *= (1 + tendencia_pm25 * 0.3);
        pred_pm10 *= (1 + tendencia_pm10 * 0.3);
        pred_o3 *= (1 + tendencia_o3 * 0.3);
        pred_co2 *= (1 + tendencia_co2 * 0.3);
        // Aplicar factores climáticos
        pred_co *= factorClimatico(clima_actual, "CO");
        pred_so2 *= factorClimatico(clima_actual, "SO2");
        pred_no2 *= factorClimatico(clima_actual, "NO2");
        pred_pm25 *= factorClimatico(clima_actual, "PM25");
        pred_pm10 *= factorClimatico(clima_actual, "PM10");
        pred_o3 *= factorClimatico(clima_actual, "O3");
        pred_co2 *= factorClimatico(clima_actual, "CO2");
        // Aplicar factores estacionales
        pred_co *= factorDiaSemana(clima_actual->dia_semana, "CO");
        pred_so2 *= factorDiaSemana(clima_actual->dia_semana, "SO2");
        pred_no2 *= factorDiaSemana(clima_actual->dia_semana, "NO2");
        pred_pm25 *= factorDiaSemana(clima_actual->dia_semana, "PM25");
        pred_pm10 *= factorDiaSemana(clima_actual->dia_semana, "PM10");
        pred_o3 *= factorDiaSemana(clima_actual->dia_semana, "O3");
        pred_co2 *= factorDiaSemana(clima_actual->dia_semana, "CO2");
        // Guardar predicciones
        predicciones[i].co_pred = pred_co;
        predicciones[i].so2_pred = pred_so2;
        predicciones[i].no2_pred = pred_no2;
        predicciones[i].pm25_pred = pred_pm25;
        predicciones[i].pm10_pred = pred_pm10;
        predicciones[i].o3_pred = pred_o3;
        predicciones[i].co2_pred = pred_co2;
        // Calcular ICA predicho
        Zona zona_temp = zonas[i];
        zona_temp.co_actual = pred_co;
        zona_temp.so2_actual = pred_so2;
        zona_temp.no2_actual = pred_no2;
        zona_temp.pm25_actual = pred_pm25;
        zona_temp.pm10_actual = pred_pm10;
        zona_temp.o3_actual = pred_o3;
        char contaminante_dom[10], categoria[50], color[20];
        float ica_pred = calcularICAZona(&zona_temp, contaminante_dom, categoria, color);
        predicciones[i].ica_predicho = ica_pred;
        strcpy(predicciones[i].categoria_predicha, categoria);
        strcpy(predicciones[i].color_predicho, color);
        strcpy(predicciones[i].contaminante_dominante_pred, contaminante_dom);
        // Calcular confianza de la predicción
        float variabilidad = 0;
        int dias_conf = registros_validos < 7 ? registros_validos : 7;
        for (int j = 0; j < dias_conf; j++) {
            float diff = fabs(co_hist[j] - pred_co) + fabs(so2_hist[j] - pred_so2) + 
                        fabs(no2_hist[j] - pred_no2) + fabs(pm25_hist[j] - pred_pm25) +
                        fabs(pm10_hist[j] - pred_pm10) + fabs(o3_hist[j] - pred_o3) +
                        fabs(co2_hist[j] - pred_co2);
            variabilidad += diff;
        }
        if (dias_conf > 0) variabilidad /= dias_conf;
        else variabilidad = 0;
        predicciones[i].confianza = 1.0 / (1.0 + variabilidad * 0.01);
        if (predicciones[i].confianza > 0.95) predicciones[i].confianza = 0.95;
        if (predicciones[i].confianza < 0.3) predicciones[i].confianza = 0.3;
    }
}

// Función para mostrar predicciones con ICA
void mostrarPrediccionesICA(Zona zonas[], PrediccionCompleta predicciones[], int cantidad) {
    printf("\n=== PREDICCIONES DE CONTAMINACION (24 HORAS) ===\n\n");
    
    for (int i = 0; i < cantidad; i++) {
        printf("Zona: %s\n", zonas[i].nombre);
        printf("----------------------------------------\n");
        
        // Mostrar valores actuales vs predichos
        printf("VALORES ACTUALES:\n");
        printf("  CO: %.2f ppm | SO2: %.2f ppb | NO2: %.2f ppb | PM2.5: %.2f ug/m3\n", 
               zonas[i].co_actual, zonas[i].so2_actual, zonas[i].no2_actual, zonas[i].pm25_actual);
        printf("  PM10: %.2f ug/m3 | O3: %.2f ppb | CO2: %.0f ppm\n", 
               zonas[i].pm10_actual, zonas[i].o3_actual, zonas[i].co2_actual);
        
        printf("PREDICCIÓN 24H:\n");
        printf("  CO: %.2f ppm | SO2: %.2f ppb | NO2: %.2f ppb | PM2.5: %.2f ug/m3\n", 
               predicciones[i].co_pred, predicciones[i].so2_pred, 
               predicciones[i].no2_pred, predicciones[i].pm25_pred);
        printf("  PM10: %.2f ug/m3 | O3: %.2f ppb | CO2: %.0f ppm\n", 
               predicciones[i].pm10_pred, predicciones[i].o3_pred, predicciones[i].co2_pred);
        
        // Mostrar cambios esperados
        printf("CAMBIOS ESPERADOS:\n");
        printf("  CO: %+.1f%% | SO2: %+.1f%% | NO2: %+.1f%% | PM2.5: %+.1f%%\n",
               ((predicciones[i].co_pred - zonas[i].co_actual) / zonas[i].co_actual) * 100,
               ((predicciones[i].so2_pred - zonas[i].so2_actual) / zonas[i].so2_actual) * 100,
               ((predicciones[i].no2_pred - zonas[i].no2_actual) / zonas[i].no2_actual) * 100,
               ((predicciones[i].pm25_pred - zonas[i].pm25_actual) / zonas[i].pm25_actual) * 100);
        printf("  PM10: %+.1f%% | O3: %+.1f%% | CO2: %+.1f%%\n",
               ((predicciones[i].pm10_pred - zonas[i].pm10_actual) / zonas[i].pm10_actual) * 100,
               ((predicciones[i].o3_pred - zonas[i].o3_actual) / zonas[i].o3_actual) * 100,
               ((predicciones[i].co2_pred - zonas[i].co2_actual) / zonas[i].co2_actual) * 100);
        
        // Mostrar ICA predicho
        printf("ICA PREDICHO: %.0f (%s - %s)\n", 
               predicciones[i].ica_predicho, predicciones[i].categoria_predicha, 
               predicciones[i].color_predicho);
        printf("Contaminante dominante esperado: %s\n", predicciones[i].contaminante_dominante_pred);
        printf("Confianza de prediccion: %.0f%%\n", predicciones[i].confianza * 100);
        
        printf("========================================\n\n");
    }
}

// Función para exportar reporte con ICA
void exportarReporte(Zona zonas[], int cantidad, const char* filename, PrediccionCompleta predicciones[]) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Error: No se pudo crear el archivo de reporte.\n");
        return;
    }
    
    fprintf(file, "=== REPORTE DE CALIDAD DEL AIRE CON ICA ===\n\n");
    
    for (int i = 0; i < cantidad; i++) {
        char contaminante_dominante[10];
        char categoria[50];
        char color[20];
        
        float ica = calcularICAZona(&zonas[i], contaminante_dominante, categoria, color);
        
        fprintf(file, "ZONA: %s\n", zonas[i].nombre);
        fprintf(file, "ICA: %.0f\n", ica);
        fprintf(file, "Categoria: %s (%s)\n", categoria, color);
        fprintf(file, "Contaminante dominante: %s\n", contaminante_dominante);
        
        fprintf(file, "\nConcentraciones actuales:\n");
        fprintf(file, "- PM2.5: %.2f μg/m³\n", zonas[i].pm25_actual);
        fprintf(file, "- PM10: %.2f μg/m³\n", zonas[i].pm10_actual);
        fprintf(file, "- CO: %.2f ppm\n", zonas[i].co_actual);
        fprintf(file, "- SO2: %.2f ppb\n", zonas[i].so2_actual);
        fprintf(file, "- NO2: %.2f ppb\n", zonas[i].no2_actual);
        fprintf(file, "- O3: %.2f ppb\n", zonas[i].o3_actual);
        
        fprintf(file, "\nCondiciones meteorologicas:\n");
        fprintf(file, "- Temperatura: %.1f°C\n", zonas[i].temperatura);
        fprintf(file, "- Viento: %.1f km/h\n", zonas[i].viento);
        fprintf(file, "- Humedad: %.1f%%\n", zonas[i].humedad);
        fprintf(file, "\n");
    }
    
    // Predicción
    fprintf(file, "\n=== PREDICCIONES DE CONTAMINACION (24 HORAS) ===\n\n");
    
    for (int i = 0; i < cantidad; i++) {
        fprintf(file, "Zona: %s\n", zonas[i].nombre);
        fprintf(file, "----------------------------------------\n");
        
        // Mostrar valores actuales vs predichos
        fprintf(file, "VALORES ACTUALES:\n");
        fprintf(file, "  CO: %.2f ppm | SO2: %.2f ppb | NO2: %.2f ppb | PM2.5: %.2f ug/m3 | PM10: %+.1f%% | O3: %+.1f%% | CO2: %+.1f%%\n", 
               zonas[i].co_actual, zonas[i].so2_actual, zonas[i].no2_actual, zonas[i].pm25_actual, zonas[i].pm10_actual , zonas[i].o3_actual, zonas[i].co2_actual);
        
        fprintf(file, "PREDICCION 24H:\n");
        fprintf(file, "  CO: %.2f ppm | SO2: %.2f ppb | NO2: %.2f ppb | PM2.5: %.2f ug/m3 | PM10: %+.1f%% | O3: %+.1f%% | CO2: %+.1f%%\n", 
               predicciones[i].co_pred, predicciones[i].so2_pred, 
               predicciones[i].no2_pred, predicciones[i].pm25_pred, predicciones[i].pm10_pred , predicciones[i].o3_pred, predicciones[i].co2_pred);
        
        // Mostrar cambios esperados (con validación para evitar división por cero)
        fprintf(file, "CAMBIOS ESPERADOS:\n");
        fprintf(file, "   CO: %.2f ppm | SO2: %.2f ppb | NO2: %.2f ppb | PM2.5: %.2f ug/m³ | PM10: %+.1f%% | O3: %+.1f%% | CO2: %+.1f%%\n",
               (zonas[i].co_actual != 0) ? ((predicciones[i].co_pred - zonas[i].co_actual) / zonas[i].co_actual) * 100 : 0,
               (zonas[i].so2_actual != 0) ? ((predicciones[i].so2_pred - zonas[i].so2_actual) / zonas[i].so2_actual) * 100 : 0,
               (zonas[i].no2_actual != 0) ? ((predicciones[i].no2_pred - zonas[i].no2_actual) / zonas[i].no2_actual) * 100 : 0,
               (zonas[i].pm25_actual != 0) ? ((predicciones[i].pm25_pred - zonas[i].pm25_actual) / zonas[i].pm25_actual) * 100 : 0,
               (zonas[i].pm10_actual != 0) ? ((predicciones[i].pm10_pred - zonas[i].pm10_actual) / zonas[i].pm10_actual) * 100 : 0,
               (zonas[i].o3_actual != 0) ? ((predicciones[i].o3_pred - zonas[i].o3_actual) / zonas[i].o3_actual) * 100 : 0,
               (zonas[i].co2_actual != 0) ? ((predicciones[i].co2_pred - zonas[i].co2_actual) / zonas[i].co2_actual) * 100 : 0);
        
        // Mostrar ICA predicho
        fprintf(file, "ICA PREDICHO: %.0f (%s - %s)\n", 
               predicciones[i].ica_predicho, predicciones[i].categoria_predicha, 
               predicciones[i].color_predicho);
        fprintf(file, "Contaminante dominante esperado: %s\n", predicciones[i].contaminante_dominante_pred);
        fprintf(file, "Confianza de prediccion: %.0f%%\n", predicciones[i].confianza * 100);
        
        fprintf(file, "========================================\n\n");
    }

    fclose(file);
    printf("Reporte exportado exitosamente a: %s\n", filename);
}