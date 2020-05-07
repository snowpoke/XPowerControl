def response(flow):
    file = open("progress.txt","a")
    file.write("\n");
    file.close();
    if "nintendo.net/v1/Game/ListWebServices" in flow.request.url:
        file = open("authorization.txt","w")
        cookie = flow.request.headers['Authorization']
        token = cookie[7:]
        file.write(token)
        file.close()
    if "nintendo.net/v1/Notification/RegisterDevice" in flow.request.url:
        file = open("registration_token.txt","w")
        content = flow.request.get_text()
        file.write(content)
        file.close()
    if "app.splatoon2.nintendo.net/api/records" in flow.request.url:
        file = open("iksm_session.txt","w")
        cookie = flow.request.headers['cookie']
        token = cookie[13:53]
        file.write(token)
        file.close()
        
