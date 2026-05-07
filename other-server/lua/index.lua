return
[[
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Chris Yockey: Software Engineer</title>
    <style>
:root {
  --bg-deep: #1d2021ff;
  --bg-base: #282828ff;
  --bg-soft: #32302fff;
  --surface: #3c3836ff;
  --surface-lift: #504945ff;
  --border: #665c54ff;
  --text-heading: #fbf1c7ff;
  --text-body: #ebdbb2ff;
  --text-subdued: #d5c4a1ff;
  --text-muted: #a89984ff;
  --text-comment: #928374ff;
  --accent-orange: #fe8019ff;
  --accent-red: #fb4934ff;
  --accent-yellow: #fabd2fff;
  --accent-green: #b8bb26ff;
  --accent-aqua: #8ec07cff;
  --accent-blue: #83a598ff;
  --accent-purple: #d3869bff;
}

body { 
  background: var(--bg-base);
}

/* header stuff */
.main-panel-header {
  display: flex;
  justify-content: space-between;
}
.header-panel-contact {
  width: 50%;
  padding: 20px;
  background: var(--bg-base);
}
.header-panel-contact { order: 1; }

#header-name { font-size: 2em; color: var(--text-body); } 
#header-title { color: var(--text-comment); }

/* contact widget */
.contact-widget {
  background-color: var(--surface-lift);
  padding: 10px;
  margin: 5px;
  border-radius: 5px;
  text-align: left;
}
.contact-widget a {
  color: var(--text-body);
  text-decoration: none;
}

/* about me */
.main-panel-about-me {
  display: flex;
  justify-content: space-between;
}
.name-title-description h1, .name-title-description h2 { margin: 2px; }
.name-title-description {
  order: 1;
  width: 33%;
}
.about-me-panel, .about-me-other {
  width: 50%;
  padding: 20px;
}
.about-me-panel { order:2; background: var(--surface-lift); }
.about-me-other { order:3; background: var(--bg-base); }
    </style>
</head>
<body>
  <div class="main-panel-header">
    <div class="header-panel-name">
      <h1 id="header-name">Chris Yockey</h1>
      <h2 id="header-title">Software Engineer</h2>
    </div>
    <div class="header-panel-contact">
      <div class="contact-widget" id="LinkedIn-widget">
        <a href="https://www.linkedin.com/in/chris-yockey/">LinkedIn</a>
      </div>
      <div class="contact-widget" id="GitHub-widget">
        <a href="https://github.com/yockey88">Github</a>
      </div>
      <div class="contact-widget" id="Email-widget">
        <a href="mailto:chrisyockey88@gmail.com">chrisyockey88@gmail.com</a>
      </div>
    </div>
  </div>
  <div class="main-panel-about-me">
    <div class="about-me-panel"></div>
    <div class="about-me-other"></div>
  </div>
</body>
</html>
]]