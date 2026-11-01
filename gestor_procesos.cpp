/* SISTEMA DE GESTION DE PROCESOS - Simulador de Planificacion de CPU (Estructura de Datos)
   Lista enlazada = registro de procesos | Cola de prioridad = CPU | Pila = memoria (max 2 GB) */
#include <iostream>
#include <cstring>
using namespace std;

const int MEMORIA_MAX = 2048;   // 2 GB expresados en MB
const char* ESTADOS[5] = {"Nuevo", "Listo", "En Ejecucion", "Bloqueado", "Terminado"};

// ---------- Estructuras de datos ----------
struct Proceso {                // nodo de la lista enlazada
    int id;
    char nombre[50];
    int prioridad;              // 1 = maxima prioridad ... 5 = minima
    char estado[20];
    int memoria;                // MB requeridos
    Proceso *sig;
};

struct NodoCola {               // nodo de la cola de prioridad
    int id;
    char nombre[50];
    int prioridad;
    NodoCola *sig;
};

struct Bloque {                 // nodo de la pila de memoria
    int idProceso;
    int mb;
    Bloque *sig;
};

// ---------- Utilidades de entrada ----------
// Lee un entero validando tipo y rango
int leerEntero(const char* msg, int min, int max) {
    int v;
    bool ok;
    do {
        cout << msg;
        cin >> v;
        ok = !cin.fail() && v >= min && v <= max;
        if (cin.fail()) cin.clear();
        cin.ignore(1000, '\n');
        if (!ok) cout << "  Entrada invalida. Rango permitido: " << min << " a " << max << "\n";
    } while (!ok);
    return v;
}

// Lee un texto no vacio (admite espacios)
void leerTexto(const char* msg, char* destino, int tam) {
    do {
        cout << msg;
        cin.getline(destino, tam);
        if (strlen(destino) == 0) cout << "  El texto no puede estar vacio.\n";
    } while (strlen(destino) == 0);
}

bool listaVacia(Proceso *cabeza) {
    return cabeza == NULL;
}

void mostrarProceso(Proceso *p) {
    cout << "  ID:" << p->id << " | " << p->nombre << " | Prioridad:" << p->prioridad
         << " | Estado:" << p->estado << " | Memoria:" << p->memoria << " MB\n";
}

// ---------- MODULO 3: PILA DE MEMORIA (LIFO) ----------
// PUSH: reserva un bloque en el tope. Valida el limite de 2 GB.
bool asignarMemoria(Bloque*& tope, int& usada, int id, int mb) {
    if (usada + mb > MEMORIA_MAX) return false;          // validacion de 2 GB
    Bloque *b = new Bloque;
    b->idProceso = id;
    b->mb = mb;
    b->sig = tope;
    tope = b;
    usada += mb;
    return true;
}

// Devuelve el bloque de un proceso (o NULL si no tiene memoria asignada)
Bloque* buscarBloque(Bloque *tope, int id) {
    Bloque *b = tope;
    while (b != NULL && b->idProceso != id) b = b->sig;
    return b;
}

// Libera el bloque de un proceso sin importar su posicion en la pila
bool liberarBloquePorId(Bloque*& tope, int& usada, int id) {
    Bloque *ant = NULL;
    Bloque *b = tope;
    while (b != NULL && b->idProceso != id) {
        ant = b;
        b = b->sig;
    }
    if (b == NULL) return false;
    if (ant == NULL) tope = b->sig; else ant->sig = b->sig;
    usada -= b->mb;
    delete b;
    return true;
}

void buscarEnMemoria(Bloque *tope) {
    int id = leerEntero("ID del proceso a buscar en memoria: ", 1, 9999);
    Bloque *b = tope;
    int pos = 1;
    while (b != NULL && b->idProceso != id) { b = b->sig; pos++; }
    if (b == NULL) cout << "  El proceso " << id << " NO tiene memoria asignada.\n";
    else cout << "  El proceso " << id << " tiene " << b->mb << " MB (posicion " << pos
              << " desde el tope).\n";
}

void mostrarEstadoMemoria(Bloque *tope, int usada) {
    cout << "\n--- ESTADO DE LA MEMORIA (tope -> base) ---\n";
    if (tope == NULL) cout << "  Pila vacia: no hay bloques asignados.\n";
    Bloque *b = tope;
    int n = 1;
    while (b != NULL) {
        cout << "  [" << n << "] Proceso " << b->idProceso << " -> " << b->mb << " MB\n";
        b = b->sig;
        n++;
    }
    cout << "  Total: " << MEMORIA_MAX << " MB | Usada: " << usada
         << " MB | Libre: " << MEMORIA_MAX - usada << " MB\n";
    int llenos = usada * 20 / MEMORIA_MAX;               // barra de 20 caracteres
    cout << "  [";
    for (int i = 0; i < 20; i++) cout << (i < llenos ? '#' : '-');
    cout << "] " << usada * 100 / MEMORIA_MAX << "%\n";
}

// ---------- MODULO 2 (parte base): COLA DE PRIORIDAD ----------
// Insercion ordenada: menor numero = mayor prioridad; a igual prioridad, por llegada
void encolarPrioridad(NodoCola*& frente, int id, const char* nom, int prio) {
    NodoCola *nuevo = new NodoCola;
    nuevo->id = id;
    strcpy(nuevo->nombre, nom);
    nuevo->prioridad = prio;
    nuevo->sig = NULL;
    if (frente == NULL || prio < frente->prioridad) {
        nuevo->sig = frente;
        frente = nuevo;
    } else {
        NodoCola *a = frente;
        while (a->sig != NULL && a->sig->prioridad <= prio) a = a->sig;
        nuevo->sig = a->sig;
        a->sig = nuevo;
    }
}

bool enCola(NodoCola *frente, int id) {
    NodoCola *a = frente;
    while (a != NULL && a->id != id) a = a->sig;
    return a != NULL;
}

// Quita un nodo de cualquier posicion de la cola (usado al eliminar o reordenar)
bool quitarDeCola(NodoCola*& frente, int id) {
    NodoCola *ant = NULL;
    NodoCola *a = frente;
    while (a != NULL && a->id != id) {
        ant = a;
        a = a->sig;
    }
    if (a == NULL) return false;
    if (ant == NULL) frente = a->sig; else ant->sig = a->sig;
    delete a;
    return true;
}

// ---------- MODULO 1: LISTA ENLAZADA (GESTOR DE PROCESOS) ----------
Proceso* buscarPorId(Proceso *cabeza, int id) {
    Proceso *a = cabeza;
    while (a != NULL && a->id != id) a = a->sig;
    return a;
}

// Cambia el estado de un proceso por su ID
bool cambiarEstado(Proceso *cabeza, int id, const char* nuevo) {
    Proceso *p = buscarPorId(cabeza, id);
    if (p == NULL) return false;
    strcpy(p->estado, nuevo);
    return true;
}

// Inserta al final de la lista y asigna su memoria (valida ID unico y 2 GB)
void insertarProceso(Proceso*& cabeza, Bloque*& tope, int& usada) {
    cout << "\n--- REGISTRAR PROCESO ---\n";
    int libre = MEMORIA_MAX - usada;
    if (libre <= 0) {
        cout << "  Error: memoria agotada (2 GB). No se pueden registrar mas procesos.\n";
        return;
    }
    int id = leerEntero("ID (1-9999): ", 1, 9999);
    if (buscarPorId(cabeza, id) != NULL) {
        cout << "  Error: ya existe un proceso con el ID " << id << ".\n";
        return;
    }
    Proceso *nuevo = new Proceso;
    nuevo->id = id;
    leerTexto("Nombre: ", nuevo->nombre, 50);
    nuevo->prioridad = leerEntero("Prioridad (1=maxima ... 5=minima): ", 1, 5);
    nuevo->memoria = leerEntero("Memoria requerida (MB): ", 1, MEMORIA_MAX);
    if (nuevo->memoria > libre) {                        // validacion de memoria
        cout << "  Error: memoria insuficiente. Solo quedan " << libre << " MB libres.\n";
        delete nuevo;
        return;
    }
    strcpy(nuevo->estado, ESTADOS[0]);
    nuevo->sig = NULL;
    if (listaVacia(cabeza)) cabeza = nuevo;
    else {
        Proceso *a = cabeza;
        while (a->sig != NULL) a = a->sig;
        a->sig = nuevo;
    }
    asignarMemoria(tope, usada, nuevo->id, nuevo->memoria);
    cout << "  Proceso registrado con estado 'Nuevo' y " << nuevo->memoria << " MB asignados.\n";
}

// Elimina un proceso de la lista, de la cola y libera su memoria
void eliminarProceso(Proceso*& cabeza, NodoCola*& frente, Bloque*& tope, int& usada) {
    if (listaVacia(cabeza)) { cout << "  No hay procesos registrados.\n"; return; }
    int id = leerEntero("ID del proceso a eliminar: ", 1, 9999);
    Proceso *ant = NULL;
    Proceso *a = cabeza;
    while (a != NULL && a->id != id) {
        ant = a;
        a = a->sig;
    }
    if (a == NULL) { cout << "  Proceso no encontrado.\n"; return; }
    if (ant == NULL) cabeza = a->sig; else ant->sig = a->sig;
    quitarDeCola(frente, id);
    liberarBloquePorId(tope, usada, id);
    cout << "  Proceso '" << a->nombre << "' eliminado y su memoria liberada.\n";
    delete a;
}

// Busqueda por ID o por nombre
void buscarProceso(Proceso *cabeza) {
    cout << "  1. Buscar por ID\n  2. Buscar por nombre\n";
    int op = leerEntero("Opcion: ", 1, 2);
    Proceso *a = cabeza;
    if (op == 1) {
        a = buscarPorId(cabeza, leerEntero("ID: ", 1, 9999));
    } else {
        char nom[50];
        leerTexto("Nombre: ", nom, 50);
        while (a != NULL && strcmp(a->nombre, nom) != 0) a = a->sig;
    }
    if (a != NULL) mostrarProceso(a);
    else cout << "  Proceso no encontrado.\n";
}

// Modifica la prioridad; si el proceso esta en la cola, la REORDENA
void modificarPrioridad(Proceso *cabeza, NodoCola*& frente) {
    int id = leerEntero("ID del proceso: ", 1, 9999);
    Proceso *p = buscarPorId(cabeza, id);
    if (p == NULL) { cout << "  Proceso no encontrado.\n"; return; }
    cout << "  Prioridad actual: " << p->prioridad << "\n";
    p->prioridad = leerEntero("Nueva prioridad (1-5): ", 1, 5);
    if (quitarDeCola(frente, id)) {                      // reordenamiento: sacar y reinsertar
        encolarPrioridad(frente, p->id, p->nombre, p->prioridad);
        cout << "  Prioridad actualizada y cola reordenada.\n";
    } else cout << "  Prioridad actualizada.\n";
}

void cambiarEstadoManual(Proceso *cabeza, NodoCola*& frente) {
    int id = leerEntero("ID del proceso: ", 1, 9999);
    if (buscarPorId(cabeza, id) == NULL) { cout << "  Proceso no encontrado.\n"; return; }
    cout << "  1. Nuevo\n  2. Listo\n  3. En Ejecucion\n  4. Bloqueado\n  5. Terminado\n";
    int e = leerEntero("Nuevo estado: ", 1, 5);
    cambiarEstado(cabeza, id, ESTADOS[e - 1]);
    if (e == 5 || e == 4) quitarDeCola(frente, id);      // bloqueado/terminado no espera CPU
    cout << "  Estado actualizado a '" << ESTADOS[e - 1] << "'.\n";
}

void listarProcesos(Proceso *cabeza) {
    cout << "\n--- LISTA GENERAL DE PROCESOS ---\n";
    if (listaVacia(cabeza)) cout << "  No hay procesos registrados.\n";
    for (Proceso *a = cabeza; a != NULL; a = a->sig) mostrarProceso(a);
}

// ---------- MODULO 3 (parte que usa la lista): POP y asignacion manual ----------
// POP: libera el bloque del tope; el proceso duenio pasa a "Terminado"
void liberarMemoria(Proceso *cabeza, NodoCola*& frente, Bloque*& tope, int& usada) {
    if (tope == NULL) { cout << "  La pila de memoria esta vacia.\n"; return; }
    Bloque *b = tope;
    tope = b->sig;
    usada -= b->mb;
    cambiarEstado(cabeza, b->idProceso, ESTADOS[4]);
    quitarDeCola(frente, b->idProceso);
    cout << "  Liberados " << b->mb << " MB del proceso " << b->idProceso
         << ". Proceso marcado como 'Terminado'.\n";
    delete b;
}

// Asigna memoria a un proceso registrado que no tenga bloque
void asignarMemoriaManual(Proceso *cabeza, Bloque*& tope, int& usada) {
    int id = leerEntero("ID del proceso: ", 1, 9999);
    Proceso *p = buscarPorId(cabeza, id);
    if (p == NULL) { cout << "  Proceso no encontrado.\n"; return; }
    if (buscarBloque(tope, id) != NULL) { cout << "  El proceso ya tiene memoria asignada.\n"; return; }
    if (!asignarMemoria(tope, usada, id, p->memoria)) {
        cout << "  Error: se excederia el limite de 2 GB.\n";
        return;
    }
    cout << "  Asignados " << p->memoria << " MB al proceso " << id << ".\n";
}

// ---------- MODULO 2: PLANIFICADOR DE CPU ----------
void planificarProceso(Proceso *cabeza, NodoCola*& frente, Bloque *tope) {
    int id = leerEntero("ID del proceso a encolar: ", 1, 9999);
    Proceso *p = buscarPorId(cabeza, id);
    if (p == NULL) { cout << "  Proceso no encontrado.\n"; return; }
    if (strcmp(p->estado, ESTADOS[4]) == 0) { cout << "  El proceso ya termino.\n"; return; }
    if (enCola(frente, id)) { cout << "  El proceso ya esta en la cola.\n"; return; }
    if (buscarBloque(tope, id) == NULL) {
        cout << "  El proceso no tiene memoria asignada (use el modulo de memoria).\n";
        return;
    }
    encolarPrioridad(frente, p->id, p->nombre, p->prioridad);
    strcpy(p->estado, ESTADOS[1]);
    cout << "  Proceso '" << p->nombre << "' encolado con estado 'Listo'.\n";
}

// Desencola el de mayor prioridad, lo ejecuta y libera su memoria
void ejecutarProcesoCPU(Proceso *cabeza, NodoCola*& frente, Bloque*& tope, int& usada) {
    if (frente == NULL) { cout << "  La cola esta vacia: no hay procesos por ejecutar.\n"; return; }
    NodoCola *n = frente;
    frente = n->sig;
    cambiarEstado(cabeza, n->id, ESTADOS[2]);
    cout << "  CPU ejecutando: " << n->nombre << " (ID " << n->id << ", prioridad "
         << n->prioridad << ")\n";
    cambiarEstado(cabeza, n->id, ESTADOS[4]);
    Bloque *b = buscarBloque(tope, n->id);
    if (b != NULL) {
        cout << "  Proceso terminado. Se liberan " << b->mb << " MB.\n";
        liberarBloquePorId(tope, usada, n->id);
    }
    delete n;
}

void buscarEnCola(NodoCola *frente) {
    int id = leerEntero("ID del proceso a buscar en la cola: ", 1, 9999);
    NodoCola *a = frente;
    int pos = 1;
    while (a != NULL && a->id != id) { a = a->sig; pos++; }
    if (a == NULL) cout << "  El proceso no esta en la cola de espera.\n";
    else cout << "  Proceso '" << a->nombre << "' en la posicion " << pos << " de la cola.\n";
}

void mostrarCola(NodoCola *frente) {
    cout << "\n--- COLA DE PLANIFICACION (frente -> final) ---\n";
    if (frente == NULL) cout << "  Cola vacia.\n";
    int pos = 1;
    for (NodoCola *a = frente; a != NULL; a = a->sig, pos++)
        cout << "  " << pos << ". ID:" << a->id << " | " << a->nombre
             << " | Prioridad:" << a->prioridad << "\n";
}

// ---------- Liberacion total de la memoria dinamica ----------
void liberarTodo(Proceso*& cabeza, NodoCola*& frente, Bloque*& tope, int& usada) {
    while (cabeza != NULL) { Proceso *t = cabeza; cabeza = cabeza->sig; delete t; }
    while (frente != NULL) { NodoCola *t = frente; frente = frente->sig; delete t; }
    while (tope != NULL) { Bloque *t = tope; tope = tope->sig; delete t; }
    usada = 0;
}

// ---------- MENUS ----------
void menuProcesos(Proceso*& cabeza, NodoCola*& frente, Bloque*& tope, int& usada) {
    int op;
    do {
        cout << "==========================================\n";
        cout << "   GESTOR DE PROCESOS (Lista Enlazada)\n";
        cout << "==========================================\n";
        cout << "  1. Registrar proceso\n";
        cout << "  2. Eliminar proceso\n";
        cout << "  3. Buscar proceso\n";
        cout << "  4. Modificar prioridad\n";
        cout << "  5. Cambiar estado\n";
        cout << "  6. Listar procesos\n";
        cout << "  0. Volver al menu principal\n";
        cout << "------------------------------------------\n";
        op = leerEntero("Opcion: ", 0, 6);
        switch (op) {
            case 1: insertarProceso(cabeza, tope, usada); break;
            case 2: eliminarProceso(cabeza, frente, tope, usada); break;
            case 3: buscarProceso(cabeza); break;
            case 4: modificarPrioridad(cabeza, frente); break;
            case 5: cambiarEstadoManual(cabeza, frente); break;
            case 6: listarProcesos(cabeza); break;
        }
        if (op != 0) system("pause");
        system("cls");
    } while (op != 0);
}

void menuCPU(Proceso *cabeza, NodoCola*& frente, Bloque*& tope, int& usada) {
    int op;
    do {
        cout << "==========================================\n";
        cout << "   PLANIFICADOR DE CPU (Cola de Prioridad)\n";
        cout << "==========================================\n";
        cout << "  1. Encolar proceso\n";
        cout << "  2. Ejecutar proceso (desencolar)\n";
        cout << "  3. Buscar en la cola\n";
        cout << "  4. Mostrar cola\n";
        cout << "  0. Volver al menu principal\n";
        cout << "------------------------------------------\n";
        op = leerEntero("Opcion: ", 0, 4);
        switch (op) {
            case 1: planificarProceso(cabeza, frente, tope); break;
            case 2: ejecutarProcesoCPU(cabeza, frente, tope, usada); break;
            case 3: buscarEnCola(frente); break;
            case 4: mostrarCola(frente); break;
        }
        if (op != 0) system("pause");
        system("cls");
    } while (op != 0);
}

void menuMemoria(Proceso *cabeza, NodoCola*& frente, Bloque*& tope, int& usada) {
    int op;
    do {
        cout << "==========================================\n";
        cout << "   GESTOR DE MEMORIA (Pila, maximo 2 GB)\n";
        cout << "==========================================\n";
        cout << "  1. Asignar memoria (Push)\n";
        cout << "  2. Liberar memoria (Pop)\n";
        cout << "  3. Buscar en memoria\n";
        cout << "  4. Mostrar estado de memoria\n";
        cout << "  0. Volver al menu principal\n";
        cout << "------------------------------------------\n";
        op = leerEntero("Opcion: ", 0, 4);
        switch (op) {
            case 1: asignarMemoriaManual(cabeza, tope, usada); break;
            case 2: liberarMemoria(cabeza, frente, tope, usada); break;
            case 3: buscarEnMemoria(tope); break;
            case 4: mostrarEstadoMemoria(tope, usada); break;
        }
        if (op != 0) system("pause");
        system("cls");
    } while (op != 0);
}

void menu() {
    Proceso *cabeza = NULL;     // lista de procesos
    NodoCola *frente = NULL;    // cola de prioridad
    Bloque *tope = NULL;        // pila de memoria
    int usada = 0;              // MB consumidos
    int op;
    do {
        cout << "==========================================\n";
        cout << "   SIMULADOR DE PLANIFICACION DE CPU\n";
        cout << "==========================================\n";
        cout << "  1. Gestor de procesos\n";
        cout << "  2. Planificador de CPU\n";
        cout << "  3. Gestor de memoria\n";
        cout << "  0. Salir\n";
        cout << "------------------------------------------\n";
        op = leerEntero("Opcion: ", 0, 3);
        system("cls");
        switch (op) {
            case 1: menuProcesos(cabeza, frente, tope, usada); break;
            case 2: menuCPU(cabeza, frente, tope, usada); break;
            case 3: menuMemoria(cabeza, frente, tope, usada); break;
            case 0: cout << "Sistema finalizado.\n"; break;
        }
    } while (op != 0);
    liberarTodo(cabeza, frente, tope, usada);             // evita fugas de memoria
}

int main() {
    menu();
    return 0;
}
