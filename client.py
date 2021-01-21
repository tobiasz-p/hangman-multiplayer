from socket import *
from tkinter import *
from tkinter import messagebox
import threading
import sys

global name
global client_sock


name = ''
user_points = []

server_ip = '127.0.0.1'
server_port = 8780


def receive_msg(sock, chat, canvas, users, points):
    print("started")
    quit = False
    user_names = ["Free slot" for _ in range(6)]
    global block, used_letter

    text = Text(chat, width=50, height=40, yscrollcommand=s.set)
    s.config(command=text.yview)
    text.pack(side=LEFT)

    while quit == False:
        try:
            msg, addr = sock.recvfrom(1024)
            message = msg.decode()
            tokens = message.split("# ")
            print(message + " FROM SERVER")  # this is to track the msgs from the server

        except ConnectionResetError:
            print("thread sorry")
            messagebox.showerror("Connection Error!", "No connection ws made...")
            break

        for i, token in enumerate(tokens):
            if token == 'available_tries':
                index = int(tokens[i + 1])
                counter = tokens[i + 2]
                canvas[index].delete("all")
                canvas[index].create_image(37, 99, image=hangman_img[int(counter)])
            elif token == 'hidden':
                block.delete(1.0, 'end')
                block.insert(INSERT, tokens[i + 1].upper())
            elif token == 'points':
                points[int(tokens[i + 1])].delete(1.0, 'end')
                points[int(tokens[i + 1])].insert(INSERT, tokens[i + 2])
            elif token == 'winner':
                messagebox.showinfo("Winner", tokens[i + 1] + " won the game!")
            elif token == 'not_started':
                messagebox.showinfo("Game not started", "Game not started yet. Press ready button!")
            elif token == 'connected':
                text.insert(INSERT, tokens[i + 2] + " connected\n")
                text.see(END)
            elif token == 'dc':
                text.insert(INSERT, user_names[int(tokens[i + 1])] + " disconnected\n")
                text.see(END)
                user_names[int(tokens[i + 1])] = "Free slot"
            elif token == 'timeout':
                text.insert(INSERT, "Timeout! Nobody answered for 20s. Skipping to next round\n")
                text.see(END)
            elif token == 'room_full':
                messagebox.showinfo("Room is full", "Room is full, please try again later!")
            elif token == 'user':
                user_names[int(tokens[i + 1])] = tokens[i + 2]
                points[int(tokens[i + 1])].delete(1.0, 'end')
                points[int(tokens[i + 1])].insert(INSERT, 0)
                update_players(user_names)
            elif token == 'wait':
                messagebox.showinfo("Wait", "Please wait for next round!")
            elif token == 'wrong_letters':
                used_letter.delete(1.0, 'end')
                letter = tokens[i + 1].upper()
                used_letter.insert(INSERT, letter)
            elif token == 'reset':
                for j, (canva, user, point) in enumerate(zip(canvas, users, points)):
                    canva.delete("all")
                    canva.create_image(37, 99, image=hangman_img[6])
                update_players(user_names)
            elif token == "message":
                text.insert(INSERT, tokens[i + 1] + "\n")
                text.see(END)
            elif token == 'stop':
                quit = True
                messagebox.showerror("Disconnected", "Connection closed.")

    print("I am done wih thread")
    chat.quit()


def update_players(user_names):
    for i, user in enumerate(users):
        user.delete(1.0, 'end')
        user.insert(INSERT, user_names[i])


def submit():
    global name
    global client_sock
    name = user.get()
    if len(name) > 0:
        try:
            client_sock = socket(AF_INET, SOCK_STREAM)
            client_sock.connect((server_ip, server_port))
            print("Connected")
            client_sock.sendall(name.encode())
            menu_w.destroy()
        except:
            messagebox.showerror("Connection error", "Unable to connect to the server. Try again later.")
        root.destroy()

    else:
        name_error()


def submit_enter(event):
    global name
    global client_sock
    name = user.get()

    if len(name) > 0:
        try:
            client_sock = socket(AF_INET, SOCK_STREAM)
            client_sock.connect((server_ip, server_port))
            print("Connected")
            client_sock.sendall(name.encode())
            menu_w.destroy()
        except:
            messagebox.showerror("Connection error", "Unable to connect to the server. Try again later.")

        root.destroy()
    else:
        name_error()


def enter(event):
    message = msg.get()
    if len(message) > 0:
        line = message
        client_sock.sendto(line.encode(), (server_ip, server_port))
        msg.delete(0, 'end')


def name_error():
    messagebox.showerror("No name!", "Please enter a user name\n")


def quit_chat():
    client_sock.close()
    chat.destroy()


def ready():
    line = "ready"
    client_sock.sendto(line.encode(), (server_ip, server_port))


def connect():
    global root, user

    root = Tk()

    root.title("Connect")
    root.maxsize(width=270, height=48)
    root.geometry("270x48+%d+%d" % (150, 250))

    user_label = Label(root, text="Enter Player Name")
    user_label.grid(row=3)

    user = Entry(root)
    user.bind("<Return>", submit_enter)
    user.grid(row=3, column=1)

    submit_button = Button(root, text="Enter", command=submit)
    submit_button.grid(row=3, column=2)

    root.mainloop()


def menu():
    global menu_w

    menu_w = Tk()

    menu_w.title("Hangman")
    menu_w.maxsize(width=1200, height=600)
    # menu_w.geometry("305x230+%d+%d" % ((500), (250)))

    img = PhotoImage(file="images/main.png")
    img_play = PhotoImage(file="images/play.png")
    img_rules = PhotoImage(file="images/rules.png")
    img_quit = PhotoImage(file="images/quit.png")

    welcome_canvas = Canvas(menu_w, width=612, height=357)
    welcome_canvas.pack(side=TOP)
    welcome_canvas.create_image(306, 178, image=img)

    play = Button(menu_w, command=connect)
    play.pack(side=LEFT)
    play.config(image=img_play, height=90, width=204)

    help = Button(menu_w, command=rules)
    help.pack(side=LEFT)
    help.config(image=img_rules, height=90, width=204)

    esc = Button(menu_w, command=quit_game)
    esc.pack(side=LEFT)
    esc.config(image=img_quit, height=90, width=204)

    menu_w.mainloop()


def rules():
    messagebox.showinfo("Help", "Rules of the game:\n\n" +
                              "* You are guessing hidden word\n\n" +
                              "* If you write more than 1 letter in textbox, only first will be checked!\n\n" +
                              "* For every guessed letter you earn 1 point\n\n" +
                              "* You have 6 trials each round. If you guess wrong, you will lose one.\n\n"
                              "* If you run out of trials you will need to wait when the round is over. " +
                              "You will also get -3 points!\n\n" +
                              "After first guess everyone has 20s for next guess. Then new word will be drawn " +
                              "and nobody will get points earned this round\n\n" +
                              "* If you have four times number of players points and everyone has less than you" +
                              " - you win.")


def quit_game():
    sys.exit(0)


if __name__ == "__main__":
    menu()
    user_length = len(name)

    if user_length > 0:
        global msg
        global canvas

        chat = Tk()

        hangman_img = []
        hangman_img.append(PhotoImage(file='images/0.png'))
        hangman_img.append(PhotoImage(file='images/1.png'))
        hangman_img.append(PhotoImage(file='images/2.png'))
        hangman_img.append(PhotoImage(file='images/3.png'))
        hangman_img.append(PhotoImage(file='images/4.png'))
        hangman_img.append(PhotoImage(file='images/5.png'))
        hangman_img.append(PhotoImage(file='images/6.png'))

        img_quit = PhotoImage(file='images/quit.png')
        img_ready = PhotoImage(file='images/ready.png')

        chat.title("Hangman game")
        chat.minsize(width=850, height=800)

        hangman = Label(chat, text="Hello, " + name + "!\n", font=("Arial", 15))
        hangman.pack(side=TOP)

        top_frame = Frame(chat, bg="dark green")
        top_frame.pack(side=TOP)

        bottom_frame = Frame(chat, height=30)
        bottom_frame.pack(side=BOTTOM)

        side_frame = Frame(top_frame, bg="dark green", bd=6, width=80, height=40)
        side_frame.pack(side=RIGHT)

        s = Scrollbar(top_frame)
        s.pack(side=RIGHT, fill=Y)

        chat_label = Label(bottom_frame, text="You: ")
        chat_label.pack(side=LEFT)

        msg = Entry(bottom_frame, width=50)
        msg.bind("<Return>", enter)
        msg.pack(side=LEFT)

        btn_ready = Button(bottom_frame, text="Ready", command=ready, height=66, width=160)
        btn_ready.pack(side=LEFT)
        btn_ready.config(image=img_ready)

        btn_quit = Button(bottom_frame, text="Quit", command=quit_chat, height=66, width=160)
        btn_quit.pack(side=RIGHT)
        btn_quit.config(image=img_quit)



        block = Text(side_frame, width=40, height=1, bg="grey")
        block.grid(row=1, column=0, columnspan=4, stick=W)
        block.insert(INSERT, "Word has not been set.")

        used_letter = Text(side_frame, width=40, height=1, bg="grey")
        used_letter.grid(row=4, columnspan=4, stick=W)
        used_letter.insert(INSERT, "Used letters will appear here...")

        canvas = [Canvas(side_frame, width=75, height=199, bg='dark green', bd=0, highlightthickness=0) for i in
                  range(6)]
        users = [Text(side_frame, width=20, height=1, bg="grey") for i in range(6)]
        points = [Text(side_frame, width=2, height=1, bg="grey") for i in range(6)]

        for i, (canva, user, point) in enumerate(zip(canvas, users, points)):
            canva.grid(row=5 + 2 * (i // 2), column=2 * (i % 2))
            canva.delete("all")
            canva.create_image(37, 99, image=hangman_img[6])
            user.grid(row=5 + 2 * (i // 2) + 1, column=2 * (i % 2), stick='W')
            user.insert(INSERT, "Free slot")
            point.grid(row=5 + 2 * (i // 2) + 1, column=2 * (i % 2) + 1, stick='W')
            point.insert(INSERT, "0")

        rt = threading.Thread(target=receive_msg, args=(client_sock, top_frame, canvas, users, points))
        rt.start()

        chat.mainloop()

        rt.join()

    client_sock.close()
