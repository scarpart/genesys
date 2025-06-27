# 



# Passo-a-passo de como criar seu próprio projeto
## Compilação
Na raíz do projeto
```console
make -C projects/GenesysTerminalApplication
```
Esse comando criará um arquivo binário em `projects/GenesysTerminalApplication/dist`. Para executar, esse arquivo precisa ter na mesma pasta o arquivo `autoloadplugins.txt`

Inicialmente, o repositório está escrito para criar uma aplicação que demonstra o funcionamento de um componente chamado `SeizeDelayRelease` pela aplicação `Smart_SeizeDelayRelease`

## Compilando o *seu* projeto
A definicação de qual aplicação será compilada está no arquivo `source/applications/terminal/TraitsTerminalApp.h`, nas linhas 17 e 58:
```cpp
#include "examples/smarts/Smart_SeizeDelayRelease.h"
.
.
.
typedef Smart_SeizeDelayRelease Application;
```

Para fazer a sua própria aplicação, basta substituir a declaração do Smart_SeizeDelayRelease pelo smart criado pelo seu grupo.

A classe `Smart_SeizeDelayRelease` está definida em `source/applications/terminal/examples/smarts/Smart_SeizeDelayRelease.cpp`. Você pode copiar esse arquivo para definir a sua própria aplicação, que, por sua vez, utilizará os componentes desenvolvidos pelo grupo para entrega do trabalho.

## Criação dos componentes
No GenESyS, "componentes" referem-se a plugins, que definirão o comportamento de cada bloco que o usuário pode usar na interface gráfica ou, como se deve fazer para o trabalho, em código que utilize o GenESyS como biblioteca.

Nos arquivos de aplicações (por exemplo o `source/applications/terminal/examples/smarts/Smart_SeizeDelayRelease.cpp`), pode-se notar a inclusão de diversos plugins das linhas 22 a 26:
```cpp
#include "../../../../plugins/components/Create.h"
#include "../../../../plugins/components/Seize.h"
#include "../../../../plugins/components/Delay.h"
#include "../../../../plugins/components/Release.h"
#include "../../../../plugins/components/Dispose.h"
```
Além disso, a modelagem de um sistema que utiliza esses componentes pode ser encontrada na implementação da classe Smart_SeizeDelayRelease nesse mesmo arquivo.

Dessa forma, basta:
- Criar o seu código fonte na pasta `source/plugins/components` definindo o comportamento dos componentes necessários para o seu respectivo trabalho.
- Incluí-los na definição da sua aplicação em `source/applications/terminal/examples/smarts/`. A sua aplicação **poderia** ser algo diferente de um smart, mas eu não sei o suficiente sobre isso pra descrever como seria qualquer outra possibilidade ¯\\\_(ツ)\_/¯
- Alterar `source/applications/terminal/TraitsTerminalApp.h` para utilizar o seu Smart em vez do Smart_SeizeDelayRelease
- Compilar o projeto com `make -C projects/GenesysTerminalApplication`
- Incluir o seu plugin na lista de plugins necessários em `/autoloadplugins.txt`
- Executar com
```bash
cd projects/GenesysTerminalApplication/dist && ./genesysterminalapplication
```