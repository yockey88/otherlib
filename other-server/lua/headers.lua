local _H = {}
_H.request_headers = {
  Accept = "Accept",
  Accept_Charset = "Accept-Charset",
  Accept_Encoding = "Accept-Encoding",
  Accept_Language = "Accept-Language",
  Authorization = "Authorization",
  Expect = "Expect",
  From = "From",
  Host = "Host",
  If_Match = "If-Match",
  If_Modified_Since = "If-Modified-Since",
  If_None_Match = "If-None-Match",
  If_Range = "If-Range",
  If_Unmodified_Since = "If-Unmodified-Since",
  Max_Forwards = "Max-Forwards",
  Proxy_Authorization = "Proxy-Authorization",
  Range = "Range",
  Referer = "Referer",
  TE = "TE",
  User_Agent = "User-Agent",
}
_H.response_headers = {
  Accept_Ranges = "Accept-Ranges",
  Age = "Age",
  ETag = "ETag",
  Location = "Location",
  Proxy_Authenticate = "Proxy-Authenticate",
  Retry_After = "Retry-After",
  Server = "Server",
  Vary = "Vary",
  WWW_Authenticate = "WWW-Authenticate"
}
return _H