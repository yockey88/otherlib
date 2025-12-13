local _StringUtils = {}

function _StringUtils.say_hello()
  print("Hello from StringUtils!")
end

function _StringUtils.strip_leading_and_ending_whitespace(str)
  return str:match("^%s*(.-)%s*$")
end

function _StringUtils.split_string_list(str)
  if str == nil or str == ""
  then
    return {}
  end

  local stripped = _StringUtils.strip_leading_and_ending_whitespace(str)
  local first_word_end = stripped:find("%s")
  if first_word_end == nil
  then
    return {}
  end

  local args_str = stripped:sub(first_word_end + 1)
  local args = {}

  local current = ""
  local in_quote = false
  local i = 1

  while i <= #args_str
  do
    local c = args_str:sub(i,i)

    if c == '"'
    then
      in_quote = not in_quote
    elseif c == ' ' and not in_quote 
    then
      if current ~= ""
      then
        table.insert(args, current)
        current = ""
      end
    else
      current = current .. c
    end
    i = i + 1
  end

  if current ~= ""
  then
    table.insert(args, current)
  end

  return args
end

return _StringUtils