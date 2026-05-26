package main

import (
	"fmt"
	"strconv"
	"strings"
	"unicode"
)

// Node kinds
type NodeKind int

const (
	NodeList   NodeKind = iota // (...)
	NodeString                 // "..."
	NodeInt                    // integer literal
	NodeFloat                  // floating-point literal
	NodeIdent                  // bare identifier/symbol
)

type Node struct {
	Kind     NodeKind
	Str      string  // NodeString, NodeIdent
	Int      int64   // NodeInt
	Flt      float64 // NodeFloat
	Children []*Node // NodeList
	Line     int
	Col      int
}

func (n *Node) String() string {
	switch n.Kind {
	case NodeList:
		parts := make([]string, len(n.Children))
		for i, c := range n.Children {
			parts[i] = c.String()
		}
		return "(" + strings.Join(parts, " ") + ")"
	case NodeString:
		return strconv.Quote(n.Str)
	case NodeInt:
		return strconv.FormatInt(n.Int, 10)
	case NodeFloat:
		return strconv.FormatFloat(n.Flt, 'f', -1, 64)
	case NodeIdent:
		return n.Str
	}
	return "<unknown>"
}

// ParseError carries source position information.
type ParseError struct {
	Line, Col int
	Msg       string
}

func (e *ParseError) Error() string {
	return fmt.Sprintf("parse error at %d:%d: %s", e.Line, e.Col, e.Msg)
}

// parser holds lexer state.
type Parser struct {
	Src  []rune
	Pos  int
	Line int
	Col  int
}

func NewParser(src string) *Parser {
	return &Parser{Src: []rune(src), Line: 1, Col: 1}
}

func (p *Parser) Peek() (rune, bool) {
	if p.Pos >= len(p.Src) {
		return 0, false
	}
	return p.Src[p.Pos], true
}

func (p *Parser) Advance() rune {
	ch := p.Src[p.Pos]
	p.Pos++
	if ch == '\n' {
		p.Line++
		p.Col = 1
	} else {
		p.Col++
	}
	return ch
}

func (p *Parser) SkipWhitespaceAndComments() {
	for {
		ch, ok := p.Peek()
		if !ok {
			return
		}
		if ch == ';' {
			// Lisp line comment: skip to end of line
			for {
				c, ok := p.Peek()
				if !ok || c == '\n' {
					break
				}
				p.Advance()
			}
		} else if unicode.IsSpace(ch) {
			p.Advance()
		} else {
			return
		}
	}
}

func (p *Parser) Errorf(line, col int, format string, args ...any) *ParseError {
	return &ParseError{Line: line, Col: col, Msg: fmt.Sprintf(format, args...)}
}

func (p *Parser) ParseString(line, col int) (*Node, error) {
	p.Advance() // consume opening "
	var sb strings.Builder
	for {
		ch, ok := p.Peek()
		if !ok {
			return nil, p.Errorf(line, col, "unterminated string")
		}
		if ch == '"' {
			p.Advance()
			break
		}
		if ch == '\\' {
			p.Advance()
			esc, ok := p.Peek()
			if !ok {
				return nil, p.Errorf(line, col, "unterminated escape sequence")
			}
			p.Advance()
			switch esc {
			case 'n':
				sb.WriteByte('\n')
			case 't':
				sb.WriteByte('\t')
			case 'r':
				sb.WriteByte('\r')
			case '"':
				sb.WriteByte('"')
			case '\\':
				sb.WriteByte('\\')
			default:
				return nil, p.Errorf(p.Line, p.Col, "unknown escape \\%c", esc)
			}
			continue
		}
		sb.WriteRune(ch)
		p.Advance()
	}
	return &Node{Kind: NodeString, Str: sb.String(), Line: line, Col: col}, nil
}

func IsIdentStart(ch rune) bool {
	return unicode.IsLetter(ch) || ch == '_' || ch == '-' || ch == '+' ||
		ch == '*' || ch == '/' || ch == '=' || ch == '<' || ch == '>' ||
		ch == '!' || ch == '?' || ch == '&' || ch == '%' || ch == '^' ||
		ch == '~' || ch == '#' || ch == '@' || ch == ':' || ch == '.'
}

func IsIdentContinue(ch rune) bool {
	return IsIdentStart(ch) || unicode.IsDigit(ch)
}

func (p *Parser) ParseAtom(line, col int) (*Node, error) {
	var sb strings.Builder
	for {
		ch, ok := p.Peek()
		if !ok || ch == '(' || ch == ')' || unicode.IsSpace(ch) || ch == ';' {
			break
		}
		sb.WriteRune(ch)
		p.Advance()
	}
	raw := sb.String()
	// Try integer first, then float.
	if i, err := strconv.ParseInt(raw, 0, 64); err == nil {
		return &Node{Kind: NodeInt, Int: i, Line: line, Col: col}, nil
	}
	if f, err := strconv.ParseFloat(raw, 64); err == nil {
		return &Node{Kind: NodeFloat, Flt: f, Line: line, Col: col}, nil
	}
	return &Node{Kind: NodeIdent, Str: raw, Line: line, Col: col}, nil
}

func (p *Parser) ParseOne() (*Node, error) {
	p.SkipWhitespaceAndComments()
	ch, ok := p.Peek()
	if !ok {
		return nil, nil // EOF
	}
	line, col := p.Line, p.Col

	switch ch {
	case ')':
		return nil, p.Errorf(line, col, "unexpected ')'")
	case '(':
		p.Advance() // consume '('
		var children []*Node
		for {
			p.SkipWhitespaceAndComments()
			next, ok := p.Peek()
			if !ok {
				return nil, p.Errorf(line, col, "unterminated list")
			}
			if next == ')' {
				p.Advance()
				break
			}
			child, err := p.ParseOne()
			if err != nil {
				return nil, err
			}
			if child == nil {
				return nil, p.Errorf(line, col, "unterminated list")
			}
			children = append(children, child)
		}
		return &Node{Kind: NodeList, Children: children, Line: line, Col: col}, nil
	case '"':
		return p.ParseString(line, col)
	default:
		return p.ParseAtom(line, col)
	}
}

// Parse parses all top-level expressions from src and returns them as a slice.
func Parse(src string) ([]*Node, error) {
	p := NewParser(src)
	var nodes []*Node
	for {
		node, err := p.ParseOne()
		if err != nil {
			return nil, err
		}
		if node == nil {
			break
		}
		nodes = append(nodes, node)
	}
	return nodes, nil
}
