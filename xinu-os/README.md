# Welcome to Xinu

# Build instructions

Copy the file compile/Makedefs.EXAMPLE to compile/Makedefs and make appropriate changes if necessary.  Make sure that the correct COMPILER_ROOT, LIBGCC_LOC and CONF_LFLAGS are set.

The PLATFORM variable should be set to one of:

- arm-qemu
- arm-bbb
- x86-qemu
- x86-galileo

## Development Environment Setup

** Running Xinu inside a Docker Container (Windows)

Please read this guideline carefully. 

DO NOT JUST COPY PASTE AND RUN EVERY COMMAND YOU SEE HERE

*** 1. Installation 

Before moving on with this tutorial, try to install docker in your machine. 

*Windows Users*:
Try to install and enable WSL2 on your machine using [[https://docs.microsoft.com/en-us/windows/wsl/install-win10][this guide]].
Then install Ubuntu from [[https://www.microsoft.com/en-us/p/ubuntu/9nblggh4msv6?activetab=pivot:overviewtab][Microsoft Store]]. 

To make sure your windows is running WSL2 not WSL1:
- Open PowerShell (Aministrator)
- Check the wsl version with `wsl -l -v`
- If the version is 1 instead of 2, use the following command to change the WSL version of Ubuntu to 2: 

```bash
Windows C:\> wsl --set-version Ubuntu-20.04 2
```

If the name is not exactly `Ubuntu-20.04` in the output of `wsl -l -v`, try to use that name instead in the command above.

- *NOTE: 20.04 is the version that was current at Microsoft at the time of writing, your version may be different.*

You may now install [[https://hub.docker.com/editions/community/docker-ce-desktop-windows][docker desktop]].
Then use item 7 of [[https://docs.docker.com/docker-for-windows/wsl/#:~:text=When%20Docker%20Desktop%20restarts%2C%20go,wsl%20%2D%2Dset%2Ddefault%20ubuntu%20.][this guide]] to enable WSL2 integration in docker deskop. 
At this point you should be able to use docker command inside the Ubuntu terminal in windows. 

Now you can clone your repository in the Ubuntu shell and follow this guideline.

*** 2. Building the Docker image

To build Xinu inside a docker container, you first need to build the docker image itself using the docker file in the xinu repository:

```bash
Windows C:\> cd xinu-$TERM/docker/student_version
Windows C:\> docker build -t xinu_image .
```

It will take some time to build the docker image. After the build is finished, you would see the public key of the container printed on the output. If you prefer to directly work inside the container, copy that and add it to your account by going to =github.iu.edu= then =settings= then =SSH and GPG keys= and =New SSH key=. Otherwise, you can use the "volume" tutorial below to edit the code on the host and commit/push your changes from the host. 

At this point you can get a list of your docker images using: 

```bash
Windows C:\> docker images
```

and you should see the =xinu_image= in the list. 

*** 3. Setup Mounting Volume (Windows users skip this)

To make your life easier for writing/editing the codes for assignments, we will create a docker volume. Create a folder at your desired location to keep the files shared with the docker container and create a folder. Here I'm assuming you want to create it in your home directory but feel free to create it in other locations (remember where you create it):

```bash
Windows C:\> mkdir ~/os_volume
```

Move or re-clone your personal repository into the new =os_volume= folder as follows:

```bash
Windows C:\> cd ~/os_volume
Windows C:\> git clone git@github.iu.edu:yourusername/yourreponame
```

*** 4. Run Image

*Running the Container*: Now you can start a container based on that image. Note that YOU SHOULD USE THIS COMMAND ONLY ONCE, otherwise you will lose your work. Note that in the following command you have to replace =xinu= with the name of the image you built before (in case it's different from =xinu_image=: 

- Run image with volume (only if you have done the previous step; Mounting Volume)

```bash
Windows C:\> docker run --name xinu_container -v ~/os_volume:/data  --cap-add=NET_ADMIN --device=/dev/net/tun -it xinu_image /bin/bash
```

- Run image without volume (Windows users)

```bash
Windows C:\> docker run --name xinu_container --cap-add=NET_ADMIN --device=/dev/net/tun -it xinu_image /bin/bash
```

*At this point you would be in the bash shell of the container.*

*** 5. Setup your xinu repository (only for Windows users)

The following step should only be done if you skipped setting up the volume section above. We will =git clone= your private xinu repository into =/data= to make the next few instructions coherent with the volume-based setup.

First, [[https://github.iu.edu/settings/ssh/new][upload the public key to GitHub]]:

```bash
docker$ cat ~/.ssh/id_rsa.pub
```

Next, clone your private repository inside your home directory (the one you are in when you log in):

```bash
docker$ git clone git@github.iu.edu:yourusername/yourreponame
```

Finally, copy over the provided =Makedefs.arm= into your =compile/Makedefs=:

```bash
docker$ cp ~/Makedefs.arm xinu-$TERM/compile/Makedefs
```

*** 6. Building Xinu Inside the Container

Now you can finally build the xinu inside the container: 

```bash
docker$ cd xinu-$TERM/compile
docker$ make -C /uboot-tool
docker$ make && make qemu
```

At this point, you should see the XINU logo inside your terminal which means you've successfully built and run xinu inside the container. 

*Exiting Xinu Shell*: To close the xinu shell use CTRL+a followed by x

*** Stopping/Starting a container

When you are done working on an assignment or part of an assignment, you can exit the container using =exit= command. Then later on, when you want to continue your work, first make sure that the container is running: 

```bash 
Windows C:\> docker ps 
```

You should see your =xinu_container= in the output. If not, DO NOT RUN A NEW CONTAINER. Use the following command to see the stopped containers: 

```bash 
Windows C:\> docker ps -a
```

If your target container is stopped, start it again using: 

```bash 
Windows C:\> docker start CONTAINER_ID
```

now you should see something like this in your =docker ps=:

```bash
Windows C:\> docker ps
CONTAINER ID        IMAGE               COMMAND             CREATED             STATUS              PORTS               NAMES
0703451f8e78        xinu                "/bin/bash"         About an hour ago   Up 6 seconds        67/tcp              xinu_container
```

Now you can connect into your container: 

```bash
Windows C:\> docker exec -it CONTAINER_ID /bin/bash
```