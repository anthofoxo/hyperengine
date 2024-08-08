project "tracy"
location "%{wks.location}/vendor/%{prj.name}"
defines "TRACY_ENABLE"
files "%{prj.location}/public/TracyClient.cpp"