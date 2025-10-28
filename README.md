# Brazo Robótico

Proyecto integrador de la materia **Electrónica Digital III**, centrado en el diseño y desarrollo de un **brazo robótico controlado mediante un microcontrolador LPC1769**.  
El sistema combina control digital, comunicación serie, conversión analógica, manejo de interrupciones y control de motores paso a paso.

---

## 🎯 Objetivo

Diseñar e implementar un brazo robótico capaz de realizar movimientos precisos y programables mediante el uso de **motores paso a paso**, controlados digitalmente a través de una **placa LPC1769**.  
El sistema debe permitir operación manual, automática y almacenamiento de movimientos, garantizando la sincronización entre los módulos y la estabilidad del control.

---

## ⚙️ Componentes principales

### 🔩 Motor paso a paso 28BYJ-48
- Tipo: unipolar  
- Número de fases: 4  
- Reducción: 1:64  
- Ángulo de paso: 5.625° / 0.175° con reducción  
- Alimentación: 5–12 VDC  
- Corriente: 240 mA  
- Torque: 0.034 N·m (0.34 kg·cm)

El control se implementó considerando **2048 pasos por vuelta**, con límites establecidos para evitar sobrecargas o giros fuera de rango.

---

## 🧠 Arquitectura del sistema

El sistema se compone de los siguientes módulos:

1. **Control de motores paso a paso**  
   - Control de dos articulaciones mediante secuencias de pasos.  
   - Conversión de ángulos ingresados a cantidad de pasos mediante:
     ```
     pasos = (grados_ingresados * 10 / GRADOS_PASO_MOTOR) * 4
     ```

2. **Interfaz de usuario**  
   - **Teclado matricial 4x4** para ingreso de comandos y ángulos.  
   - Implementación de *debounce* por software (55 ms) para estabilidad de lectura.  

3. **Temporizadores (Timer 0 y Timer 1)**  
   - **Timer 1:** escaneo del teclado (período de 10 ms).  
   - **Timer 0:** sincronización del muestreo ADC (frecuencia de 20 Hz).

4. **ADC y control de velocidad**  
   - Potenciómetro de 10 kΩ conectado al pin **P0.23** para ajustar la velocidad de movimiento.  
   - Conversión analógica utilizada para modificar dinámicamente la frecuencia de los pasos del motor.  
   - Rango operativo: **50 Hz a 300 Hz**.  
     ```
     delay[Hz] = 0.06 * velocidad + 50
     ```

5. **Comunicación UART2**  
   - Interfaz serie con la PC mediante **CP2102 USB-UART Bridge**.  
   - Configuración: 9600 bps, 8 N 1, sin paridad.  
   - Transmisión gestionada mediante **GPDMA canal 0** (memoria a periférico).

6. **Manejo de interrupciones**
   - Prioridad 1: `EINT3_IRQHandler()` → teclado.  
   - Prioridad 2: `TIMER1_IRQHandler()` → escaneo de teclado.  
   - Prioridad 3: `ADC_IRQHandler()` → lectura analógica.  
   - Control coordinado para evitar conflictos entre eventos simultáneos.

---

## ⚡ Funcionamiento general

- El usuario ingresa los comandos o ángulos de movimiento a través del teclado matricial.  
- El sistema convierte los valores a pasos de motor, controlando las bobinas de los 28BYJ-48.  
- La velocidad del movimiento se ajusta dinámicamente con el potenciómetro.  
- La comunicación UART permite visualizar o registrar los estados y movimientos.  
- El sistema puede operar en modo **manual**, **automático** o **guardado**.

---

## 🧩 Tecnologías utilizadas

- **Microcontrolador:** NXP LPC1769  
- **Lenguaje:** C  
- **IDE:** MCUXpresso / LPCXpresso  
- **Módulos:** UART2, ADC, GPDMA, Timer0/1, GPIO, EINT3  
- **Componentes externos:** motores paso a paso 28BYJ-48, teclado 4x4, potenciómetro 10 kΩ

---

## 🔬 Resultados

El brazo robótico logra movimientos suaves y controlados en ambos ejes, con respuesta inmediata a las entradas del teclado.  
El control de velocidad mediante ADC es estable y proporcional, y la comunicación serie permite monitorear los valores en tiempo real.  
El sistema demuestra una integración efectiva entre hardware, periféricos y software embebido.
