---
layout: default
---
### Downloads

The current release is **{{ site.releases.first.version }}** released **{{ site.releases.first.date | date: "%B %-d, %Y" }}**.

[**Download SDK for Windows**]({{ site.s3_url }}/sdks/Carbon%20SDK%20{{ site.releases.first.version }}.exe)  
[**Download SDK for Mac OS X & iOS**]({{ site.s3_url }}/sdks/Carbon%20SDK%20{{ site.releases.first.version }}.pkg)

##### SDK Contents

- Release and debug builds of Carbon
- Sample applications
- Public headers and library files needed for development
- Project wizards for Visual Studio and Xcode
- Asset exporters for 3D Studio Max and Maya (Windows only)

##### Changelog

For details of the changes made in each release see [`CHANGELOG.md`]({{ site.github_url }}/blob/master/CHANGELOG.md).

---

##### Older Releases

{% for release in site.releases %}
  {% unless forloop.first %}
* Version **{{ release.version }}** released **{{ release.date | date: "%B %-d, %Y" }}**.  

  [Download SDK for Windows]({{ site.s3_url }}/sdks/Carbon%20SDK%20{{ release.version }}.exe)  
  [Download SDK for Mac OS X & iOS]({{ site.s3_url }}/sdks/Carbon%20SDK%20{{ release.version }}.pkg)
  {% endunless %}
{% endfor %}
