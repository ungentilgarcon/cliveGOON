from pygments import highlight
from pygments.lexers.compiled import CLexer
from pygments.formatters import HtmlFormatter

print "<pre>"
try:
  while True:
    line = raw_input()
    if (line[:4] == 'diff') or (line[:5] == 'index') or (line[:3] == '---') or (line[:3] == '+++'):
      print ('<span class="dh">' + line + '</span>')
    elif line[:2] == '@@': # FIXME highlight code after second @@
      print ('<span class="dl">' + line + '</span>')
    elif line[:1] == ' ':
      print ('<span class="du"> ' + highlight(line[1:], CLexer(), HtmlFormatter(nowrap=True))[:-1] + '</span>')
    elif line[:1] == '+':
      print ('<span class="di">+' + highlight(line[1:], CLexer(), HtmlFormatter(nowrap=True))[:-1] + '</span>')
    elif line[:1] == '-':
      print ('<span class="dd">-' + highlight(line[1:], CLexer(), HtmlFormatter(nowrap=True))[:-1] + '</span>')
    else:
      print ('<span class="dw">' + line + '</span>')
except EOFError:
  pass
print "</pre>"
