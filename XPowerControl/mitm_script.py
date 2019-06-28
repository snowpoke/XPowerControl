def response(flow):
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