---
# Instalação do NS-3.38 com NetSimulyzer no Ubuntu 22.04

Este guia fornece o passo a passo para a instalação do simulador de redes NS-3 (versão 3.38) e da ferramenta de visualização 3D NetSimulyzer em um ambiente Linux Ubuntu 22.04 LTS.

---

## 1. Preparação do Sistema Operacional

**Pré-requisito:**
Certifique-se de estar utilizando a seguinte versão do Linux:
* **SO:** Ubuntu 22.04.5 LTS (Jammy Jellyfish)
* **Download ISO:** [ubuntu-22.04.5-desktop-amd64.iso](https://releases.ubuntu.com/jammy/ubuntu-22.04.5-desktop-amd64.iso)

### Configuração Inicial e Permissões
Caso esteja utilizando uma Máquina Virtual (como VirtualBox) ou precise configurar o usuário `sudo` pela primeira vez:

```bash
su -
adduser vboxuser sudo
exit
reboot
````

> **Explicação:** O comando `adduser` garante que o seu usuário tenha permissões de administrador para instalar pacotes. O `reboot` é necessário para aplicar as alterações de grupo.

-----

## 2\. Instalação do NS-3.38

Primeiro, instalamos todas as dependências necessárias (compiladores, bibliotecas Python, ferramentas de build e bibliotecas gráficas).

### 2.1 Instalar Dependências

Execute os comandos abaixo para atualizar o sistema e baixar os pacotes:

```bash
sudo apt update

sudo apt install g++ python3 cmake ninja-build git gir1.2-goocanvas-2.0 python3-gi python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython3 tcpdump wireshark sqlite sqlite3 libsqlite3-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools openmpi-bin openmpi-common openmpi-doc libopenmpi-dev doxygen graphviz imagemagick python3-sphinx dia texlive dvipng latexmk texlive-extra-utils texlive-latex-extra texlive-font-utils libeigen3-dev gsl-bin libgsl-dev libgslcblas0 libxml2 libxml2-dev libgtk-3-dev lxc-utils lxc-templates vtun uml-utilities ebtables bridge-utils libboost-all-dev
```

### 2.2 Download e Compilação do NS-3

Faça o download do código fonte, extraia na sua pasta `home` e compile:

```bash
# Baixar e extrair o NS-3.38
wget [https://www.nsnam.org/releases/ns-allinone-3.38.tar.bz2](https://www.nsnam.org/releases/ns-allinone-3.38.tar.bz2)
tar -xjf ns-allinone-3.38.tar.bz2

# Entrar na pasta
cd ns-allinone-3.38/

# Compilar o simulador com exemplos e testes ativados
./build.py --enable-examples --enable-tests 
```

### 2.3 Testar a Instalação

Verifique se a instalação ocorreu bem rodando o script de exemplo:

```bash
cd ns-3.38
./ns3 hello-simulator
```

-----

## 3\. Instalação do NetSimulyzer

O NetSimulyzer é dividido em duas partes: o **Módulo** (que roda dentro do NS-3) e o **Visualizador** (software 3D independente).

### 3.1 Instalar Dependências do NetSimulyzer (Qt5 e Qt6)

O NetSimulyzer exige bibliotecas gráficas específicas para renderização 3D.

```bash
sudo apt update

# Ferramentas básicas e Qt5
sudo apt install -y build-essential g++ python3 python3-dev python3-pip cmake git unzip qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools libxml2-dev libqt5charts5-dev graphviz imagemagick

# Módulos 3D do Qt5
sudo apt install -y qt3d5-dev qt3d5-dev-tools libqt53dcore5 libqt53drender5 libqt53dinput5 libqt53dlogic5 libqt53dextras5

# Módulos do Qt6 (para compatibilidade futura ou build específico)
sudo apt install -y qt6-base-dev qt6-3d-dev libqt63dcore6 libqt63drender6 libqt63dinput6 libqt63dlogic6 libqt63dextras6 qmake6
```

### 3.2 Instalar o Módulo NetSimulyzer no NS-3

Agora, baixamos o módulo e recompilamos o NS-3 para reconhecê-lo.

```bash
# Navegar até a pasta de módulos de terceiros (contrib)
cd ns-allinone-3.38/ns-3.38/contrib

# Baixar o módulo específico (versão 1.0.7)
wget [https://github.com/usnistgov/NetSimulyzer-ns3-module/archive/refs/tags/v1.0.7.zip](https://github.com/usnistgov/NetSimulyzer-ns3-module/archive/refs/tags/v1.0.7.zip) -O NetSimulyzer-ns3-module-master.zip

# Extrair e renomear a pasta (passo obrigatório)
unzip NetSimulyzer-ns3-module-master.zip
mv NetSimulyzer-ns3-module-1.0.7 netsimulyzer
rm NetSimulyzer-ns3-module-master.zip

# Retornar à pasta raiz do ns-3.38
cd ..

# Reconfigurar e recompilar o NS-3
./ns3 configure --enable-examples --enable-tests
./ns3 build -j 1
```

> **Nota:** O comando `./ns3 build -j 1` compila usando apenas 1 núcleo do processador. Isso é mais lento, mas evita que o computador trave por falta de memória RAM durante a compilação pesada do módulo 3D.

### 3.3 Instalar o Visualizador (NetSimulyzer 3D Viewer)

Por fim, instalamos o aplicativo que abre os arquivos gerados pela simulação.

```bash
# Voltar para a pasta Home (saindo do diretório ns-allinone)
cd ..
cd ..
cd .. 

# Clonar o repositório do visualizador
git clone --recursive [https://github.com/usnistgov/NetSimulyzer.git](https://github.com/usnistgov/NetSimulyzer.git)

# Criar pasta de build e compilar
cd NetSimulyzer
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 1

# Executar o visualizador
./netsimulyzer
```
---
