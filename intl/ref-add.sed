/^# Packages using this file: / {
  s/# Packages using this file://
  ta
  :a
  s/ maketool / maketool /
  tb
  s/ $/ maketool /
  :b
  s/^/# Packages using this file:/
}
