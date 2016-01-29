---
layout: default
---
### Screenshots

&nbsp;

{% for screenshot in site.screenshots %}

[![]({{ site.assets_url }}/screenshots/{{ screenshot.filename }})]({{ site.assets_url }}/screenshots/{{ screenshot.filename }})
<p style="text-align: center">
{{ screenshot.text }}
</p>
&nbsp;

{% endfor %}
