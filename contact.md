---
layout: default
---
### Contact

##### Bugs & Feature Requests

All bug reports and feature requests for Carbon are handled in [GitHub Issues]({{ site.github_url }}/issues).

##### People

Please be in touch if you have any queries about developing with Carbon.

{% for person in site.people %}

[{{ person.name }}]({{ person.url }})  
**{{ person.role }}**

{% endfor %}
