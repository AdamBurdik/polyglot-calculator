import re
import sys

NUMBER = re.compile(r"(?:\d+\.\d*|\.\d+|\d+)(?:[eE][+-]?\d+)?")
NAME = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")
OPS = {"+": "add", "-": "sub", "*": "mul", "/": "div", "^": "pow"}


def tokenize(source):
    tokens = []
    i = 0
    while i < len(source):
        ch = source[i]
        if ch.isspace():
            i += 1
            continue
        if ch.isdigit() or ch == ".":
            match = NUMBER.match(source, i)
            if match is None:
                raise ValueError("bad number at position %d" % i)
            tokens.append(("num", match.group(0)))
            i = match.end()
            continue
        if ch.isalpha() or ch == "_":
            match = NAME.match(source, i)
            if match is None:
                raise ValueError("unexpected character %r" % ch)
            tokens.append(("name", match.group(0)))
            i = match.end()
            continue
        if source.startswith("**", i):
            tokens.append(("op", "^"))
            i += 2
            continue
        if ch in "+-*/^(),":
            tokens.append(("op", ch))
            i += 1
            continue
        raise ValueError("unexpected character %r" % ch)
    tokens.append(("end", None))
    return tokens


class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0
        self.out = []

    def peek(self):
        return self.tokens[self.pos]

    def eat(self, kind, value=None):
        token = self.tokens[self.pos]
        if token[0] != kind or (value is not None and token[1] != value):
            raise ValueError("expected %s" % (value if value is not None else kind))
        self.pos += 1
        return token[1]

    def emit_num(self, value):
        self.out.append("n " + value)

    def emit_op(self, name):
        self.out.append("o " + name)

    def parse(self):
        self.expr()
        if self.peek()[0] != "end":
            raise ValueError("unexpected token %r" % (self.peek()[1],))
        return self.out

    def expr(self):
        self.term()
        while self.peek() in (("op", "+"), ("op", "-")):
            op = OPS[self.eat("op")]
            self.term()
            self.emit_op(op)

    def starts_value(self):
        kind, value = self.peek()
        return kind in ("num", "name") or (kind, value) == ("op", "(")

    def term(self):
        self.unary()
        while self.peek() in (("op", "*"), ("op", "/")) or self.starts_value():
            if self.peek() in (("op", "*"), ("op", "/")):
                op = OPS[self.eat("op")]
            else:
                op = "mul"
            self.unary()
            self.emit_op(op)

    def unary(self):
        if self.peek() == ("op", "-"):
            self.eat("op")
            self.unary()
            self.emit_op("neg")
        elif self.peek() == ("op", "+"):
            self.eat("op")
            self.unary()
        else:
            self.power()

    def power(self):
        self.atom()
        if self.peek() == ("op", "^"):
            self.eat("op")
            self.unary()
            self.emit_op("pow")

    def atom(self):
        kind, value = self.peek()
        if kind == "num":
            self.eat("num")
            self.emit_num(value)
        elif kind == "name":
            name = self.eat("name")
            if self.peek() == ("op", "("):
                self.eat("op")
                if self.peek() != ("op", ")"):
                    self.expr()
                    while self.peek() == ("op", ","):
                        self.eat("op")
                        self.expr()
                self.eat("op", ")")
            self.out.append("o " + name)
        elif (kind, value) == ("op", "("):
            self.eat("op")
            self.expr()
            self.eat("op", ")")
        else:
            raise ValueError("expected a value")


def find_order(source):
    parser = Parser(tokenize(source))
    return "\n".join(parser.parse())


def main():
    if len(sys.argv) < 2:
        sys.stderr.write("EXPRESSION\n")  # neo: find_order.py EXPRESSION
        return 2
    try:
        sys.stdout.write(find_order(sys.argv[1]) + "\n")
    except ValueError as error:
        sys.stderr.write("error: %s\n" % error)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
