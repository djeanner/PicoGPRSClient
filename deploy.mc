🌐 4. Deploy on Render

    Go to https://render.com

    Create an account (or log in)

    Click "New +" > "Web Service"

    Choose "Deploy from a Git repository"

    Connect your GitHub and select your repo

    In the "Environment" section:

        Set Runtime to Docker

        Set Dockerfile Path to Dockerfile

        Set Port to 3000

    Click "Create Web Service"

🎉 Render will build and deploy your Docker container. After a minute or two, it will show you a public URL like:

https://air780-server.onrender.com

📡 Use Your Server
To submit data from AIR780:

Send POST to:

POST https://air780-server.onrender.com/submit
Body: device=AIR780&value=123

curl -X POST https://air780-server.onrender.com/submit \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "device=AIR780&value=123"

To retrieve data from your laptop:

wget https://air780-server.onrender.com/data -O -
wget https://air780-server.onrender.com/data -O filenName.txt
curl https://air780-server.onrender.com/data

