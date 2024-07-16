# shell

### Búsqueda en $PATH

La syscall `execve(2)` y la familia `exec(3)` ofrecen formas de ejecutar un nuevo programa dentro de un proceso en un sistema operativo basado en Unix. Aunque ambas realizan tareas similares, difieren en varios aspectos clave. Principalmente, se diferencian en cómo se encuentra el programa, cómo se especifican los argumentos y de dónde proviene el entorno. 

Por un lado, `execve(2)` es una llamada al sistema (bajo nivel) que permite ejecutar un nuevo programa especificando la ruta completa del archivo ejecutable. Esta llamada directa al kernel del sistema operativo proporciona un control preciso sobre la ejecución del programa. Sin embargo, requiere que el programador maneje explícitamente todos los detalles, como la construcción del array de argumentos y variables de entorno.

Por otro lado, la familia de funciones `exec(3)` envuelve la llamada `execve(2)` para ofrecer una experiencia de programación más conveniente y segura. Estas funciones, como `execl`, `execle`, `execlp`, `execv`, `execvp`, entre otras, permiten especificar los argumentos y variables de entorno de formas más simples, como listas de cadenas de caracteres o arrays de punteros.

En base a la documentación, vamos con algunos ejemplos y su relación con sus respectivos sufijos:
- Las llamadas con `v` en el nombre toman un *vector* para especificar los argumentos del nuevo programa. El final de los argumentos se indica mediante un elemento de matriz que contiene `NULL`.
- Las llamadas con `l` en el nombre toman a los argumentos del nuevo programa como una lista de argumentos de longitud variable. El final de los argumentos se indica mediante un argumento `(char *)NULL`.
- Las llamadas con `e` en el nombre toman un argumento adicional (o argumentos en el caso de `l`) para proporcionar el entorno del nuevo programa; de lo contrario, el programa hereda el entorno del proceso actual.
- Las llamadas con `p` en el nombre buscan la variable de entorno PATH para encontrar el programa si no tiene un directorio (es decir, no contiene un carácter `/`). De lo contrario, el nombre del programa siempre se trata como una ruta al ejecutable.

Cabe destacar una diferencia importante entre `execve(2)` y `exec(3)`, la cual radica en la compatibilidad con la resolución de rutas de archivos ejecutables. Mientras que `execve(2)` no realiza ninguna búsqueda en el PATH, requiriendo la especificación precisa de la ruta completa del archivo, algunas variantes de `exec(3)` realizan automáticamente esta búsqueda, lo que facilita la ejecución de programas sin preocuparse por la ubicación exacta del archivo ejecutable.

Ahora bien, hablemos de los fallos de estas llamadas. La llamada `exec(3)` puede fallar, y, en esos casos, se devuelve un número negativo (-1) y un `errno` para indicar el error. La shell informa al usuario y, dependiendo de la implementación, por ejemplo, podría volver al estado de espera de la entrada del usuario para que pueda intentar otro comando.

Estos errores pueden obtenerse si el archivo ejecutable especificado no existe, si el usuario no tiene permisos suficientes para ejecutar cierto archivo, si hay problemas con el formato del archivo ejecutable o bien si el sistema no tiene los recursos necesarios para cargar y ejecutar el programa.

De acuerdo con la documentación de Linux, se especifica lo siguiente:
- En caso de que se deniegue el permiso para un archivo y la llamada a `execve(2)` falle con el error EACCES, las funciones relacionadas seguirán buscando en el resto de la ruta especificada pero si no encuentran otro archivo disponible, retornarán indicando el error EACCES.
- Si el encabezado de un archivo no es reconocido y la llamada a `execve(2)` falla con el error ENOEXEC, estas funciones ejecutarán el shell (/bin/sh) utilizando la ruta del archivo como primer argumento. Si este intento también resulta fallido, no se realizarán más búsquedas.

### Procesos en segundo plano

Primeramente ejecutamos el proceso de forma normal. Hacemos `fork(2)`, creando un hijo del proceso padre. Luego, en el proceso hijo, ejecutamos el programa con `exec(2)`.
La diferencia con el foreground es que en background el proceso padre seguirá su ejecución sin llamar a `wait()`, por lo que podremos seguir escribiendo comandos en la shell mientras hay procesos hijo ejecutándose en segundo plano. Cuando algún proceso hijo termine, enviará una señal de `SIGCHILD` a la shell para indicar que él mismo terminó. En caso de que este proceso tenga group-id de los procesos background el handler capturará la señal e imprimirá el id del proceso finalizado. Esto es posible ya que en el handler llamamos a waitpid con el flag `WNOHANG`, evitando bloquear la shell si el proceso hijo no terminó.

En caso de no usar `Señales` podemos llamar a waitpid con el flag `WNOHANG` oportunisticamente (por ejemplo cada vez que imprimimos el prompt), esto funciona pero no es la mejor opcion ya que no liberamos los recursos hasta ese momento.

El uso de señales en programación es necesario para permitir la comunicación asíncrona entre procesos o entre partes de un mismo programa. Las señales son eventos asíncronos que pueden ser enviados por el sistema operativo, por otros procesos, o incluso por el mismo proceso.

### Flujo estándar

`2>&1` es utilizada para redirigir la salida de error estándar hacia el mismo destino que la salida estándar, haciendo que tanto el stderr y el stdout acaben en un mismo lugar.
Por partes:
- "2" es el file descritor de la salida de error estándar
- ">" es el operador de redirección de salida
- "&1" es el file descriptor de la salida estándar. "&" aclara que lo siguiente (1) es un file descriptor y no el nombre de un archivo.

Supongamos que tenemos el siguiente comando:
```
ls directorio_inexistente 2>&1 > out.txt
```
En este caso, ls directorio_inexistente intentará listar un directorio que no existe, lo que generará un mensaje de error en la salida de errores estándar (stderr). Sin embargo, mediante `2>&1`, estamos redirigiendo este mensaje de error hacia la salida estándar (stdout), que a su vez es redirigida al archivo out.txt.

Entonces, el archivo out.txt contendrá tanto la salida estándar (si hubiera alguna) como los mensajes de error generados por el comando ls, ya que ambos flujos de salida se han redirigido al mismo destino.

Sin embargo, si invertimos el orden de la redirección, es decir:
```
ls directorio_inexistente > out.txt 2>&1
```
En este caso, primero redirigimos la salida de errores estándar hacia la salida estándar y luego redirigimos la salida estándar al archivo out.txt. Esto significa que solo la salida estándar se escribirá en el archivo out.txt, mientras que los mensajes de error se imprimirán en la terminal. Esto se debe a que la redirección de la salida estándar al archivo out.txt se produce después de que hayamos redirigido la salida de errores estándar hacia la salida estándar.

### Tuberías múltiples

Cuando ejecutamos un pipe en la shell, el exit code reportado por la shell generalmente refleja el estado de la última operación realizada en el pipe. Sin embargo, el comportamiento puede variar dependiendo de si los comandos dentro del pipe tienen éxito o fallan.

Si alguno de los comandos dentro del pipe falla, la shell devuelve el error de todas las ejecuciones fallidas. Esto significa que si uno de los comandos en el pipe falla, el resultado final será un código de salida no exitoso, indicando que algo salió mal en la ejecución del pipe.

Vamos con algunos ejemplos.

- Ambos comandos fallan:
```
$ ls /noexiste | grep z
ls: /noexiste: No such file or directory
cat: z: No such file or directory
```
En este caso, tanto ls como cat fallan. La shell devuelve el error de ambas ejecuciones.

Sin embargo, si solo uno de los comandos dentro del pipe falla, la salida del pipe todavía puede ser exitosa si el último comando se ejecuta correctamente. En este caso, la shell reportará el código de salida del último comando ejecutado.

- Un comando falla:
```
$ ls /noexiste | echo hello
hello
ls: /noexiste: No such file or directory
```
Aquí, el comando ls falla, pero echo hello se ejecuta correctamente. La shell devuelve el código de salida de echo hello, que es exitoso.

- Expliquemos a detalle este ejemplo particular:
```
$ echo hello > | cat noexiste
cat: noexiste: No such file or directory
```
En este caso, luego del echo, el operador > redirige esa salida estándar hacia un archivo llamado noexiste, utilizando la redirección de salida. Pero, el símbolo | inmediatamente después de > indica el inicio de un pipe, lo que significa que la salida del comando echo hello se redirigirá al comando siguiente en el pipe en lugar de a un archivo llamado noexiste.

Como noexiste no es un comando válido y se interpreta como el nombre de un archivo, el comando cat intenta leer el contenido de un archivo llamado noexiste. Sin embargo, este archivo no existe, por lo que cat falla y muestra un mensaje de error que indica que no puede encontrar el archivo especificado.

Por lo tanto, la salida de echo hello se pierde en este caso, ya que se intenta redirigir a un archivo que no se crea, y en su lugar se muestra un mensaje de error de cat debido a la redirección mal formada.

### Variables de entorno temporarias

En primer lugar, es necesario hacer la configuración de las variables de entorno luego de la llamada a `fork(2)` para asegurar que tanto el proceso padre como el hijo tengan acceso a las mismas variables y que los cambios realizados en una no afecten al otro.

El comportamiento que obtendrias haciendo el `setenv()` podria ser distinto al que obtendrias pasandole las variables por argumento, ya que en este último caso solo se crearán las variables de entorno pasadas sin tener en cuenta las variables de entorno del proceso anterior. Por lo que para obtener el mismo comportamiento necesitariamos hacer una copia exacta de las variables de entorno del proceso padre.

Para lograr un comportamiento similar al de `setenv()`, se podría implementar una función que copie las variables de entorno del proceso padre a un arreglo temporal y luego agregar las nuevas variables necesarias. Este arreglo se pasaría como tercer argumento a las funciones de la familia `exec(3)`. 

La clave es asegurarse de que el proceso hijo tenga acceso a las mismas variables de entorno que el proceso padre, si es necesario, o de lo contrario, podrías perder información crucial o introducir comportamientos inesperados en la ejecución del programa.

### Pseudo-variables

- `$#`: Esta variable almacena el número de argumentos pasados a un script o función.
- `$@`: Almacena todos los argumentos pasados a un script como una lista separada por espacios. Cada argumento se trata como una entidad separada, lo que significa que si un argumento contiene espacios, estos se considerarán como parte del mismo argumento y no como delimitadores.

```
#!/bin/bash
echo "Número de argumentos pasados: $#"
echo "Los argumentos pasados son: $@"
```
En este ejemplo, `$#` mostrará el número de argumentos pasados al script cuando se ejecute. Por otro lado, `$@` expandirá todos los argumentos pasados como una lista separada por espacios.

- `$$`: Esta variable almacena el ID de proceso (PID) del script en ejecución. Esto significa que devuelve un valor único que identifica el proceso del script en el sistema operativo. Es útil cuando necesitas hacer referencia al proceso en sí mismo.

```
#!/bin/bash
echo "El ID de proceso del script es: $$"
``` 

Cuando ejecutas este script, verás el PID del proceso actual del script. Por ejemplo, si el PID es 1234, la salida sería:
```
El ID de proceso del script es: 1234
```

### Comandos built-in

La funcion `cd` no podria implementarse sin ser built-in, ya que si se ejecutara en un proceso hijo sin ser built-in, solo modificaría el directorio de trabajo del proceso hijo, no del padre, lo que no tendría el efecto deseado.

En cambio, la función `pwd` si podría implementarse sin built-in. Esto es porque después de producirse el fork, el padre y el hijo ambos se encontrarían en el mismo directorio. Por lo tanto, al imprimirse el `pdw`, se imprimiría lo mismo en el padre que en el hijo. Cabe destacar que esta función se implementa como built-in ya que no haría el forkeo y, por lo tanto, mejoraría la velocidad y la posibilidad de errores en el mismo. 

### Historial

---
