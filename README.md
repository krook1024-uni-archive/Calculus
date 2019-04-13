<img align="right" width="136" height="68" src="https://www.gnu.org/graphics/gplv3-with-text-136x68.png">

# 🔴🔵 Calculus

Two players play this game in which there are three stacks of
small rocks (hence the name; *Calculus*). The number of rocks in
a stack is the same for all three stacks. One round in this game
means both players taking an amount of rocks which can be between
one and a number defined at the start of the game. The winner is the
one taking the last rock.

This is a project of mine written from scratch for a class
called **Hálózati architektúrák és protokollok** which stands for
Network architectures and protocolls.

## TODO

**Server (C):**

- [x] Argument processing
- [x] Basic server-client communication
- [x] Implement game rules
- [x] Resignation given the word `feladom` or `resign`
- [x] Read messages from the client and act accordingly
- [x] Recognize a win, let the users know, and exit (*exiting is required by the project*)
- [ ] Send current battle's data to the client if requested

**Client (Python):**

- [ ] Basic server-client communication (connect, disconnect)
- [ ] Build a basic GUI with [Kivy](https://kivy.org)
- [ ] Connect to the server
- [ ] Act accordingly to user input
