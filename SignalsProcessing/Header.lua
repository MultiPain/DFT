sin = math.sin
cos = math.cos
tan = math.tan
acos = math.acos
asin = math.asin
atan = math.atan
atan2 = math.atan2
abs = math.abs
sqrt = math.sqrt
exp = math.exp
log = math.log
pi = math.pi
pi2 = pi * 2
pi_2 = pi / 2
pi_r = 1 / pi

ceil = math.ceil
floor = math.floor
fmod = math.fmod
max = math.max
min = math.min
random = math.random
randomseed = math.randomseed
rad = math.rad
deg = math.deg

function sign(a)
	return a < 0 and -1 or 1
end

function saw(t)
	return (t % pi2) * pi_r - 1
end

function square(t)
	local v = sin(t)
	return sign(v)
end

function triangle(t)
    return 2 * abs((t - pi_2) % (pi2) * pi_r - 1) - 1
end

randomseed(10000)